package org.jetbrains.skiko.context

import org.jetbrains.skia.DirectContext
import org.jetbrains.skia.GLAssembledInterface
import org.jetbrains.skia.makeGLWithInterface
import org.jetbrains.skiko.SkiaLayer

internal class WaylandOpenGLContextHandler(layer: SkiaLayer) : OpenGLContextHandler(layer) {
    private var glInterface: GLAssembledInterface? = null

    override fun makeContext(): DirectContext {
        val interfacePtr = getEGLGetProcAddressFunc()
        glInterface = GLAssembledInterface.createFromNativePointers(0L, interfacePtr)
        return DirectContext.makeGLWithInterface(glInterface!!)
    }

    override fun dispose() {
        super.dispose()
        glInterface?.close()
        glInterface = null
    }
}

private external fun getEGLGetProcAddressFunc(): Long
