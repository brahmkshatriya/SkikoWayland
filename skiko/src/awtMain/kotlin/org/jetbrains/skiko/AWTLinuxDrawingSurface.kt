package org.jetbrains.skiko

import java.awt.Toolkit

internal inline fun <T> HardwareLayer.lockLinuxDrawingSurface(action: (LinuxDrawingSurface) -> T): T {
    val drawingSurface = lockLinuxDrawingSurface(this)
    try {
        return action(drawingSurface)
    } finally {
        unlockLinuxDrawingSurface(drawingSurface)
    }
}

internal fun lockLinuxDrawingSurface(layer: HardwareLayer): LinuxDrawingSurface {
    val drawingSurface = layer.getDrawingSurface()
    drawingSurface.lock()
    return drawingSurface.getInfo().use {
        if (isWaylandToolkit()) {
            WaylandDrawingSurface(
                common = drawingSurface,
                display = getWaylandDisplay(it.platformInfo),
                parentSurface = getWaylandParentSurface(it.platformInfo),
                x = getWaylandX(it.platformInfo),
                y = getWaylandY(it.platformInfo),
                width = getWaylandWidth(it.platformInfo),
                height = getWaylandHeight(it.platformInfo),
                scale = getWaylandScale(it.platformInfo),
                effectiveScale = getWaylandEffectiveScale(it.platformInfo),
                javaX = getWaylandJavaX(it.platformInfo),
                javaY = getWaylandJavaY(it.platformInfo),
                javaWidth = getWaylandJavaWidth(it.platformInfo),
                javaHeight = getWaylandJavaHeight(it.platformInfo),
            )
        } else {
            X11DrawingSurface(
                common = drawingSurface,
                display = getDisplay(it.platformInfo),
                window = getWindow(it.platformInfo)
            )
        }
    }
}

internal fun unlockLinuxDrawingSurface(drawingSurface: LinuxDrawingSurface) {
    drawingSurface.common.unlock()
    drawingSurface.common.close()
}

internal sealed interface LinuxDrawingSurface {
    val common: DrawingSurface
}

internal data class X11DrawingSurface(
    override val common: DrawingSurface,
    val display: Long,
    val window: Long
) : LinuxDrawingSurface

internal data class WaylandDrawingSurface(
    override val common: DrawingSurface,
    val display: Long,
    val parentSurface: Long,
    val x: Int,
    val y: Int,
    val width: Int,
    val height: Int,
    val scale: Int,
    val effectiveScale: Double,
    val javaX: Int,
    val javaY: Int,
    val javaWidth: Int,
    val javaHeight: Int
) : LinuxDrawingSurface

internal fun isWaylandToolkit(): Boolean =
    Toolkit.getDefaultToolkit().javaClass.name == "sun.awt.wl.WLToolkit"

private external fun getDisplay(platformInfo: Long): Long
private external fun getWindow(platformInfo: Long): Long
private external fun getWaylandDisplay(platformInfo: Long): Long
private external fun getWaylandParentSurface(platformInfo: Long): Long
private external fun getWaylandX(platformInfo: Long): Int
private external fun getWaylandY(platformInfo: Long): Int
private external fun getWaylandWidth(platformInfo: Long): Int
private external fun getWaylandHeight(platformInfo: Long): Int
private external fun getWaylandScale(platformInfo: Long): Int
private external fun getWaylandEffectiveScale(platformInfo: Long): Double
private external fun getWaylandJavaX(platformInfo: Long): Int
private external fun getWaylandJavaY(platformInfo: Long): Int
private external fun getWaylandJavaWidth(platformInfo: Long): Int
private external fun getWaylandJavaHeight(platformInfo: Long): Int
