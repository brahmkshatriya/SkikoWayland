package org.jetbrains.skiko.redrawer

import kotlin.math.roundToInt
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.cancel
import org.jetbrains.skiko.FrameDispatcher
import org.jetbrains.skiko.GraphicsApi
import org.jetbrains.skiko.LayerDrawScope
import org.jetbrains.skiko.Logger
import org.jetbrains.skiko.MainUIDispatcher
import org.jetbrains.skiko.OpenGLApi
import org.jetbrains.skiko.RenderException
import org.jetbrains.skiko.SkiaLayer
import org.jetbrains.skiko.SkiaLayerAnalytics
import org.jetbrains.skiko.SkiaLayerProperties
import org.jetbrains.skiko.SkikoProperties
import org.jetbrains.skiko.WaylandDrawingSurface
import org.jetbrains.skiko.context.WaylandOpenGLContextHandler
import org.jetbrains.skiko.hostOs
import org.jetbrains.skiko.isVideoCardSupported
import org.jetbrains.skiko.layerFrameLimiter
import org.jetbrains.skiko.loadOpenGLLibrary
import org.jetbrains.skiko.lockLinuxDrawingSurface
import org.jetbrains.skiko.unlockLinuxDrawingSurface

internal class LinuxWaylandEGLRedrawer(
    private val layer: SkiaLayer,
    analytics: SkiaLayerAnalytics,
    private val properties: SkiaLayerProperties
) : AWTRedrawer(layer, analytics, GraphicsApi.OPENGL) {
    init {
        loadOpenGLLibrary()
    }

    private val contextHandler = WaylandOpenGLContextHandler(layer)
    override val renderInfo: String get() = contextHandler.rendererInfo()

    private var context = 0L
    private val swapInterval = if (properties.isVsyncEnabled) 1 else 0
    private var lastSurfaceTrace: String? = null
    private var drawBufferWidth = 1
    private var drawBufferHeight = 1

    init {
        layer.backedLayer.lockLinuxDrawingSurface {
            val surface = it as WaylandDrawingSurface
            val frame = surface.toRenderFrame()
            context = createContext(surface, frame, layer.transparency)
            if (context == 0L) {
                throw RenderException("Cannot create Linux Wayland EGL context")
            }
            updateSurface(frame, context)
            makeCurrent(context)
            adapterName.also { adapterName ->
                if (adapterName != null && !isVideoCardSupported(GraphicsApi.OPENGL, hostOs, adapterName)) {
                    throw RenderException("Cannot create Linux Wayland EGL context")
                }
            }
            onDeviceChosen(adapterName)
            if (System.getProperty("skiko.hardwareInfo.enabled") == "true") {
                Logger.info {
                    "LinuxWaylandEGLRedrawer initialized: " +
                        "display=0x${surface.display.toString(16)}, " +
                        "parentSurface=0x${surface.parentSurface.toString(16)}, " +
                        "bounds=${surface.x},${surface.y} ${surface.width}x${surface.height}, " +
                        "scale=${surface.scale}, effectiveScale=${surface.effectiveScale}, renderer=$adapterName"
                }
            }
            setSwapInterval(context, swapInterval)
        }
        onContextInit()
    }

    private val adapterName get() = OpenGLApi.instance.glGetString(OpenGLApi.instance.GL_RENDERER)

    private val frameJob = Job()
    @Volatile
    private var frameLimit = 0.0
    private val frameLimiter = layerFrameLimiter(
        CoroutineScope(frameJob),
        layer.backedLayer,
        onNewFrameLimit = { frameLimit = it }
    )

    private suspend fun limitFramesIfNeeded() {
        if (properties.isVsyncEnabled) {
            try {
                frameLimiter.awaitNextFrame()
            } catch (e: CancellationException) {
                // ignore
            }
        }
    }

    override fun dispose() {
        checkDisposed()
        frameJob.cancel()
        makeCurrent(context)
        contextHandler.dispose()
        destroyContext(context)
        super.dispose()
    }

    override fun needRender(throttledToVsync: Boolean) {
        checkDisposed()
        toRedraw.add(this)
        frameDispatcher.scheduleFrame()
    }

    override fun renderImmediately() = layer.backedLayer.lockLinuxDrawingSurface {
        val surface = it as WaylandDrawingSurface
        checkDisposed()
        update()
        val frame = surface.toRenderFrame()
        surface.traceUpdate(frame)
        updateSurface(frame, context)
        inWaylandDrawScope(frame) {
            makeCurrent(context)
            drawFrame()
            val turnOffVsync = properties.isVsyncEnabled && !SkikoProperties.linuxWaitForVsyncOnRedrawImmediately
            if (turnOffVsync) {
                setSwapInterval(context, 0)
            }
            swapBuffers(context)
            OpenGLApi.instance.glFlush()
            if (turnOffVsync) {
                setSwapInterval(context, swapInterval)
            }
        }
    }

    private fun draw() {
        LayerDrawScope(layer.pixelGeometry, drawBufferWidth, drawBufferHeight).drawFrame()
    }

    private fun org.jetbrains.skiko.LayerDrawScope.drawFrame() {
        OpenGLApi.instance.glViewport(0, 0, scaledLayerWidth, scaledLayerHeight)
        contextHandler.draw()
    }

    private fun WaylandDrawingSurface.traceUpdate(frame: WaylandRenderFrame) {
        if (System.getProperty("skiko.hardwareInfo.enabled") != "true") {
            return
        }
        val trace = "layer=${layer.width}x${layer.height}, " +
            "contentScale=${layer.contentScale}, " +
            "surface=${x},${y} ${width}x${height}, " +
            "java=${javaX},${javaY} ${javaWidth}x${javaHeight}, " +
            "buffer=${frame.bufferWidth}x${frame.bufferHeight}, " +
            "destination=${frame.destinationWidth}x${frame.destinationHeight}, " +
            "scale=$scale, effectiveScale=$effectiveScale"
        if (trace != lastSurfaceTrace) {
            lastSurfaceTrace = trace
            Logger.info { "LinuxWaylandEGLRedrawer surface update: $trace" }
        }
    }

    private fun WaylandDrawingSurface.toRenderFrame(): WaylandRenderFrame {
        val renderScale = effectiveScale.toFloat().takeIf { it > 0f } ?: layer.contentScale
        val bufferWidth = (layer.width * renderScale).roundToInt().coerceAtLeast(1)
        val bufferHeight = (layer.height * renderScale).roundToInt().coerceAtLeast(1)
        return WaylandRenderFrame(
            renderSurface = parentSurface,
            destinationWidth = width.coerceAtLeast(1),
            destinationHeight = height.coerceAtLeast(1),
            bufferWidth = bufferWidth,
            bufferHeight = bufferHeight,
            javaX = javaX,
            javaY = javaY,
            javaWidth = javaWidth,
            javaHeight = javaHeight
        )
    }

    private inline fun inWaylandDrawScope(frame: WaylandRenderFrame, body: LayerDrawScope.() -> Unit) {
        drawBufferWidth = frame.bufferWidth
        drawBufferHeight = frame.bufferHeight
        LayerDrawScope(layer.pixelGeometry, drawBufferWidth, drawBufferHeight).body()
    }

    private fun createContext(surface: WaylandDrawingSurface, frame: WaylandRenderFrame, transparency: Boolean) =
        createContext(
            surface.display,
            frame.renderSurface,
            frame.destinationWidth,
            frame.destinationHeight,
            frame.bufferWidth,
            frame.bufferHeight,
            transparency
        )

    private fun updateSurface(frame: WaylandRenderFrame, context: Long) {
        drawBufferWidth = frame.bufferWidth
        drawBufferHeight = frame.bufferHeight
        updateSurface(
            context,
            frame.renderSurface,
            frame.destinationWidth,
            frame.destinationHeight,
            frame.bufferWidth,
            frame.bufferHeight
        )
    }

    companion object {
        private val toRedraw = mutableSetOf<LinuxWaylandEGLRedrawer>()
        private val toRedrawCopy = mutableSetOf<LinuxWaylandEGLRedrawer>()
        private val toRedrawVisible = toRedrawCopy
            .asSequence()
            .filterNot(LinuxWaylandEGLRedrawer::isDisposed)
            .filter { it.layer.isShowing }

        private val frameDispatcher = FrameDispatcher(MainUIDispatcher) {
            toRedrawCopy.addAll(toRedraw)
            toRedraw.clear()

            toRedrawVisible.maxByOrNull { it.frameLimit }?.limitFramesIfNeeded()

            val nanoTime = System.nanoTime()
            for (redrawer in toRedrawVisible) {
                try {
                    redrawer.update(nanoTime)
                } catch (e: CancellationException) {
                    // continue
                }
            }

            val drawingFrames = toRedrawVisible.associateWith {
                val surface = lockLinuxDrawingSurface(it.layer.backedLayer) as WaylandDrawingSurface
                surface to it.run { surface.toRenderFrame() }
            }
            try {
                for (redrawer in toRedrawVisible) {
                    drawingFrames[redrawer]!!.also { (surface, frame) ->
                        redrawer.run { surface.traceUpdate(frame) }
                        redrawer.updateSurface(frame, redrawer.context)
                    }
                    makeCurrent(redrawer.context)
                    redrawer.draw()
                }

                val vsyncRedrawer = toRedrawVisible
                    .filter { it.properties.isVsyncEnabled }
                    .maxByOrNull { it.frameLimit }

                for (redrawer in toRedrawVisible.filter { it != vsyncRedrawer }) {
                    setSwapInterval(redrawer.context, 0)
                    swapBuffers(redrawer.context)
                    OpenGLApi.instance.glFlush()
                }

                if (vsyncRedrawer != null) {
                    setSwapInterval(vsyncRedrawer.context, 1)
                    swapBuffers(vsyncRedrawer.context)
                    OpenGLApi.instance.glFlush()
                }
            } finally {
                drawingFrames.values.forEach { unlockLinuxDrawingSurface(it.first) }
            }

            toRedrawCopy.clear()
        }
    }
}

private data class WaylandRenderFrame(
    val renderSurface: Long,
    val destinationWidth: Int,
    val destinationHeight: Int,
    val bufferWidth: Int,
    val bufferHeight: Int,
    val javaX: Int,
    val javaY: Int,
    val javaWidth: Int,
    val javaHeight: Int
)

private external fun createContext(
    display: Long,
    parentSurface: Long,
    width: Int,
    height: Int,
    bufferWidth: Int,
    bufferHeight: Int,
    transparency: Boolean
): Long

private external fun updateSurface(
    context: Long,
    parentSurface: Long,
    width: Int,
    height: Int,
    bufferWidth: Int,
    bufferHeight: Int
)

private external fun makeCurrent(context: Long)
private external fun destroyContext(context: Long)
private external fun setSwapInterval(context: Long, interval: Int)
private external fun swapBuffers(context: Long)
