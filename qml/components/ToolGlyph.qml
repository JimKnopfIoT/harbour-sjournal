import QtQuick 2.0
import Sailfish.Silica 1.0

Canvas {
    id: glyph

    property string kind: "pen"
    property color color: Theme.primaryColor
    property real strokeScale: 0.09
    property int corners: 3

    // Opaque strokes, faded as one item: translucent ones double up where they cross.
    readonly property color ink: Qt.rgba(color.r, color.g, color.b, 1.0)

    opacity: color.a

    implicitWidth: Theme.iconSizeMedium
    implicitHeight: Theme.iconSizeMedium

    antialiasing: true
    renderStrategy: Canvas.Cooperative

    onKindChanged: requestPaint()
    onColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onCornersChanged: requestPaint()

    // A Canvas comes back blank from the background; onPaint does not fire again.
    onAvailableChanged: if (available) requestPaint()
    onVisibleChanged: if (visible) requestPaint()

    Connections {
        target: Qt.application
        onStateChanged: {
            if (Qt.application.state === Qt.ApplicationActive)
                glyph.requestPaint()
        }
    }

    onPaint: {
        var ctx = getContext("2d")
        var s = Math.min(width, height)
        ctx.reset()
        ctx.save()
        ctx.translate((width - s) / 2, (height - s) / 2)
        ctx.scale(s, s)

        ctx.strokeStyle = ink
        ctx.fillStyle = ink
        ctx.lineWidth = strokeScale
        ctx.lineCap = "round"
        ctx.lineJoin = "round"

        if (kind === "pen") {
            ctx.beginPath()
            ctx.moveTo(0.80, 0.14)
            ctx.lineTo(0.94, 0.28)
            ctx.lineTo(0.40, 0.82)
            ctx.lineTo(0.16, 0.90)
            ctx.lineTo(0.24, 0.66)
            ctx.closePath()
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.24, 0.66)
            ctx.lineTo(0.40, 0.82)
            ctx.stroke()
        } else if (kind === "marker") {
            ctx.beginPath()
            ctx.moveTo(0.78, 0.10)
            ctx.lineTo(0.94, 0.26)
            ctx.lineTo(0.46, 0.74)
            ctx.lineTo(0.24, 0.74)
            ctx.lineTo(0.24, 0.52)
            ctx.closePath()
            ctx.stroke()
            ctx.globalAlpha = 0.45
            ctx.beginPath()
            ctx.moveTo(0.14, 0.90)
            ctx.lineTo(0.62, 0.90)
            ctx.lineWidth = strokeScale * 2.2
            ctx.stroke()
            ctx.globalAlpha = 1.0
        } else if (kind === "bezier") {
            ctx.beginPath()
            ctx.moveTo(0.12, 0.76)
            ctx.bezierCurveTo(0.34, 0.18, 0.66, 0.88, 0.88, 0.28)
            ctx.stroke()
            ctx.lineWidth = strokeScale * 0.55
            ctx.beginPath()
            ctx.moveTo(0.12, 0.76); ctx.lineTo(0.34, 0.18)
            ctx.moveTo(0.88, 0.28); ctx.lineTo(0.66, 0.88)
            ctx.stroke()
            ctx.lineWidth = strokeScale
            var nr = 0.075
            ctx.fillRect(0.12 - nr, 0.76 - nr, 2 * nr, 2 * nr)
            ctx.fillRect(0.88 - nr, 0.28 - nr, 2 * nr, 2 * nr)
            ctx.beginPath()
            ctx.arc(0.34, 0.18, 0.06, 0, 2 * Math.PI)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(0.66, 0.88, 0.06, 0, 2 * Math.PI)
            ctx.stroke()
        } else if (kind === "brush") {
            ctx.beginPath()
            ctx.moveTo(0.80, 0.08)
            ctx.lineTo(0.94, 0.22)
            ctx.lineTo(0.60, 0.56)
            ctx.lineTo(0.46, 0.42)
            ctx.closePath()
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.46, 0.42)
            ctx.bezierCurveTo(0.26, 0.58, 0.24, 0.74, 0.14, 0.90)
            ctx.bezierCurveTo(0.34, 0.82, 0.48, 0.78, 0.60, 0.56)
            ctx.closePath()
            ctx.stroke()
        } else if (kind === "eraser") {
            ctx.beginPath()
            ctx.moveTo(0.58, 0.12)
            ctx.lineTo(0.90, 0.44)
            ctx.lineTo(0.48, 0.86)
            ctx.lineTo(0.16, 0.86)
            ctx.lineTo(0.16, 0.54)
            ctx.closePath()
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.32, 0.38)
            ctx.lineTo(0.64, 0.70)
            ctx.stroke()
        } else if (kind === "move" || kind === "pan") {
            var c = 0.5, a = 0.86, b = 0.14, h = 0.10
            ctx.beginPath()
            ctx.moveTo(c, b); ctx.lineTo(c, a)
            ctx.moveTo(b, c); ctx.lineTo(a, c)
            ctx.stroke()
            var tips = [[c, b, 0, 1], [c, a, 0, -1], [b, c, 1, 0], [a, c, -1, 0]]
            for (var i = 0; i < tips.length; ++i) {
                var t = tips[i]
                ctx.beginPath()
                ctx.moveTo(t[0], t[1])
                ctx.lineTo(t[0] + t[2] * h - t[3] * h, t[1] + t[3] * h - t[2] * h)
                ctx.moveTo(t[0], t[1])
                ctx.lineTo(t[0] + t[2] * h + t[3] * h, t[1] + t[3] * h + t[2] * h)
                ctx.stroke()
            }
        } else if (kind === "layers") {
            for (var k = 0; k < 3; ++k) {
                var y = 0.30 + k * 0.22
                ctx.beginPath()
                ctx.moveTo(0.50, y - 0.16)
                ctx.lineTo(0.90, y)
                ctx.lineTo(0.50, y + 0.16)
                ctx.lineTo(0.10, y)
                ctx.closePath()
                ctx.globalAlpha = k === 0 ? 1.0 : 0.55
                ctx.stroke()
            }
            ctx.globalAlpha = 1.0
        } else if (kind === "photo") {
            ctx.beginPath()
            ctx.rect(0.10, 0.20, 0.80, 0.60)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.16, 0.72)
            ctx.lineTo(0.40, 0.44)
            ctx.lineTo(0.58, 0.64)
            ctx.lineTo(0.68, 0.54)
            ctx.lineTo(0.84, 0.72)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(0.68, 0.34, 0.07, 0, 2 * Math.PI)
            ctx.stroke()
        } else if (kind === "shape") {
            ctx.beginPath()
            ctx.rect(0.10, 0.16, 0.42, 0.42)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(0.66, 0.64, 0.24, 0, 2 * Math.PI)
            ctx.stroke()
        } else if (kind === "select") {
            var sl = 0.14, st = 0.20, sr = 0.86, sb = 0.80
            var dash = 0.10, gap = 0.06
            function dashedLine(x1, y1, x2, y2) {
                var ddx = x2 - x1, ddy = y2 - y1
                var dl = Math.sqrt(ddx * ddx + ddy * ddy)
                var ux2 = ddx / dl, uy2 = ddy / dl
                for (var t = 0; t < dl; t += dash + gap) {
                    var e = Math.min(t + dash, dl)
                    ctx.beginPath()
                    ctx.moveTo(x1 + ux2 * t, y1 + uy2 * t)
                    ctx.lineTo(x1 + ux2 * e, y1 + uy2 * e)
                    ctx.stroke()
                }
            }
            dashedLine(sl, st, sr, st)
            dashedLine(sr, st, sr, sb)
            dashedLine(sr, sb, sl, sb)
            dashedLine(sl, sb, sl, st)
        } else if (kind === "circle") {
            ctx.beginPath()
            ctx.ellipse ? ctx.ellipse(0.12, 0.24, 0.76, 0.52, 0, 0, 2 * Math.PI)
                        : ctx.arc(0.5, 0.5, 0.34, 0, 2 * Math.PI)
            ctx.stroke()
        } else if (kind === "square") {
            ctx.beginPath()
            ctx.rect(0.14, 0.22, 0.72, 0.56)
            ctx.stroke()
        } else if (kind === "polygon") {
            var pn = Math.max(3, corners)
            ctx.beginPath()
            for (var v = 0; v <= pn; ++v) {
                var pa = -Math.PI / 2 + 2 * Math.PI * v / pn
                var px = 0.5 + 0.36 * Math.cos(pa)
                var py = 0.5 + 0.36 * Math.sin(pa)
                v === 0 ? ctx.moveTo(px, py) : ctx.lineTo(px, py)
            }
            ctx.closePath()
            ctx.stroke()
        } else if (kind === "star") {
            var sn = Math.max(3, corners)
            var inner = sn <= 3 ? 0.20
                      : sn === 4 ? 0.30
                                 : Math.max(0.3, Math.min(0.75,
                                     Math.cos(2 * Math.PI / sn) / Math.cos(Math.PI / sn)))
            ctx.beginPath()
            for (var w2 = 0; w2 <= 2 * sn; ++w2) {
                var sa = -Math.PI / 2 + Math.PI * w2 / sn
                var f = (w2 % 2 === 0) ? 1.0 : inner
                var sx = 0.5 + 0.40 * f * Math.cos(sa)
                var sy = 0.5 + 0.40 * f * Math.sin(sa)
                w2 === 0 ? ctx.moveTo(sx, sy) : ctx.lineTo(sx, sy)
            }
            ctx.closePath()
            ctx.stroke()
        } else if (kind === "lock") {
            ctx.beginPath()
            ctx.rect(0.20, 0.46, 0.60, 0.44)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(0.50, 0.46, 0.20, Math.PI, 2 * Math.PI, false)
            ctx.stroke()
        } else if (kind === "outline") {
            ctx.lineWidth = strokeScale * 2.6
            ctx.beginPath()
            ctx.arc(0.32, 0.5, 0.22, 0, 2 * Math.PI)
            ctx.stroke()
            ctx.lineWidth = strokeScale * 0.45
            ctx.beginPath()
            ctx.arc(0.72, 0.5, 0.22, 0, 2 * Math.PI)
            ctx.stroke()
            ctx.lineWidth = strokeScale
        } else if (kind === "group") {
            var gl = 0.08, gt = 0.14, gr = 0.92, gb = 0.86
            var gd = 0.11, gg = 0.07
            function gdash(x1, y1, x2, y2) {
                var ddx = x2 - x1, ddy = y2 - y1
                var dl = Math.sqrt(ddx * ddx + ddy * ddy)
                var ux3 = ddx / dl, uy3 = ddy / dl
                for (var t = 0; t < dl; t += gd + gg) {
                    var e = Math.min(t + gd, dl)
                    ctx.beginPath()
                    ctx.moveTo(x1 + ux3 * t, y1 + uy3 * t)
                    ctx.lineTo(x1 + ux3 * e, y1 + uy3 * e)
                    ctx.stroke()
                }
            }
            ctx.lineWidth = strokeScale * 0.8
            gdash(gl, gt, gr, gt); gdash(gr, gt, gr, gb)
            gdash(gr, gb, gl, gb); gdash(gl, gb, gl, gt)
            ctx.lineWidth = strokeScale
            ctx.beginPath()
            ctx.rect(0.20, 0.28, 0.30, 0.30)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(0.66, 0.60, 0.17, 0, 2 * Math.PI)
            ctx.stroke()
        } else if (kind === "copy") {
            ctx.beginPath()
            ctx.rect(0.12, 0.12, 0.50, 0.58)
            ctx.stroke()
            ctx.beginPath()
            ctx.rect(0.36, 0.32, 0.50, 0.58)
            ctx.stroke()
        } else if (kind === "paste") {
            ctx.beginPath()
            ctx.rect(0.18, 0.22, 0.64, 0.66)
            ctx.stroke()
            ctx.beginPath()
            ctx.rect(0.36, 0.10, 0.28, 0.20)
            ctx.stroke()
        } else if (kind === "cut") {
            ctx.beginPath()
            ctx.moveTo(0.22, 0.12); ctx.lineTo(0.66, 0.66)
            ctx.moveTo(0.78, 0.12); ctx.lineTo(0.34, 0.66)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(0.28, 0.78, 0.13, 0, 2 * Math.PI)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(0.72, 0.78, 0.13, 0, 2 * Math.PI)
            ctx.stroke()
        } else if (kind === "trash") {
            ctx.beginPath()
            ctx.moveTo(0.16, 0.26); ctx.lineTo(0.84, 0.26)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.38, 0.26); ctx.lineTo(0.38, 0.16)
            ctx.lineTo(0.62, 0.16); ctx.lineTo(0.62, 0.26)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.24, 0.26); ctx.lineTo(0.30, 0.88)
            ctx.lineTo(0.70, 0.88); ctx.lineTo(0.76, 0.26)
            ctx.stroke()
        } else if (kind === "magnet") {
            ctx.beginPath()
            ctx.arc(0.5, 0.52, 0.30, Math.PI, 2 * Math.PI, false)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.20, 0.52); ctx.lineTo(0.20, 0.80)
            ctx.moveTo(0.80, 0.52); ctx.lineTo(0.80, 0.80)
            ctx.stroke()
            ctx.lineWidth = strokeScale * 1.8
            ctx.beginPath()
            ctx.moveTo(0.20, 0.78); ctx.lineTo(0.20, 0.86)
            ctx.moveTo(0.80, 0.78); ctx.lineTo(0.80, 0.86)
            ctx.stroke()
            ctx.lineWidth = strokeScale
        } else if (kind === "text" || kind === "textfilled"
                   || kind === "textoutline" || kind === "textinline") {
            ctx.beginPath()
            ctx.moveTo(0.16, 0.88)
            ctx.lineTo(0.50, 0.12)
            ctx.lineTo(0.84, 0.88)
            ctx.moveTo(0.29, 0.62)
            ctx.lineTo(0.71, 0.62)
            if (kind === "textfilled") {
                ctx.lineWidth = strokeScale * 2.6
                ctx.stroke()
                ctx.lineWidth = strokeScale
            } else if (kind === "textinline") {
                ctx.lineWidth = strokeScale * 2.6
                ctx.stroke()
                ctx.lineWidth = strokeScale * 0.7
                ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.85)
                ctx.stroke()
                ctx.strokeStyle = ink
                ctx.lineWidth = strokeScale
            } else {
                ctx.stroke()
            }
        } else if (kind === "ruler") {
            ctx.beginPath()
            ctx.rect(0.10, 0.34, 0.80, 0.32)
            ctx.stroke()
            for (var tk = 1; tk < 4; ++tk) {
                var tx = 0.10 + 0.80 * tk / 4
                ctx.beginPath()
                ctx.moveTo(tx, 0.34)
                ctx.lineTo(tx, tk === 2 ? 0.56 : 0.47)
                ctx.stroke()
            }
        } else if (kind === "line") {
            ctx.beginPath()
            ctx.moveTo(0.14, 0.82)
            ctx.lineTo(0.86, 0.18)
            ctx.stroke()
        } else if (kind === "polyline" || kind === "spline" || kind === "splineclosed") {
            var nodes = kind === "splineclosed"
                    ? [[0.50, 0.14], [0.86, 0.46], [0.66, 0.88], [0.30, 0.82], [0.14, 0.44]]
                    : [[0.12, 0.74], [0.38, 0.26], [0.62, 0.76], [0.88, 0.28]]

            ctx.beginPath()
            ctx.moveTo(nodes[0][0], nodes[0][1])
            if (kind === "polyline") {
                for (var pl = 1; pl < nodes.length; ++pl)
                    ctx.lineTo(nodes[pl][0], nodes[pl][1])
            } else {
                var shut = kind === "splineclosed"
                var count = shut ? nodes.length : nodes.length - 1
                function node(i) {
                    return shut ? nodes[((i % nodes.length) + nodes.length) % nodes.length]
                                : nodes[Math.max(0, Math.min(nodes.length - 1, i))]
                }
                for (var sp = 0; sp < count; ++sp) {
                    var b = node(sp - 1), f = node(sp), t = node(sp + 1), a2 = node(sp + 2)
                    ctx.bezierCurveTo(f[0] + (t[0] - b[0]) / 6, f[1] + (t[1] - b[1]) / 6,
                                      t[0] - (a2[0] - f[0]) / 6, t[1] - (a2[1] - f[1]) / 6,
                                      t[0], t[1])
                }
                if (shut)
                    ctx.closePath()
            }
            ctx.stroke()

            for (var nd = 0; nd < nodes.length; ++nd) {
                ctx.beginPath()
                ctx.arc(nodes[nd][0], nodes[nd][1], 0.065, 0, 2 * Math.PI)
                ctx.fill()
            }
        } else if (kind === "arrow") {
            var ax = 0.14, ay = 0.82, bx = 0.84, by = 0.20
            var adx = bx - ax, ady = by - ay
            var alen = Math.sqrt(adx * adx + ady * ady)
            var aux = adx / alen, auy = ady / alen
            var head = 0.28, arad = 25 * Math.PI / 180
            ctx.beginPath()
            ctx.moveTo(ax, ay)
            ctx.lineTo(bx, by)
            ctx.stroke()
            for (var sgn = -1; sgn <= 1; sgn += 2) {
                var ca = Math.cos(sgn * arad), sa = Math.sin(sgn * arad)
                ctx.beginPath()
                ctx.moveTo(bx, by)
                ctx.lineTo(bx - head * (aux * ca - auy * sa),
                           by - head * (aux * sa + auy * ca))
                ctx.stroke()
            }
        } else if (kind === "zoom") {
            ctx.beginPath()
            ctx.arc(0.44, 0.42, 0.26, 0, 2 * Math.PI)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.63, 0.61)
            ctx.lineTo(0.88, 0.86)
            ctx.lineWidth = strokeScale * 1.4
            ctx.stroke()
            ctx.lineWidth = strokeScale
            ctx.beginPath()
            ctx.moveTo(0.30, 0.42)
            ctx.lineTo(0.58, 0.42)
            ctx.moveTo(0.44, 0.28)
            ctx.lineTo(0.44, 0.56)
            ctx.stroke()
        } else if (kind === "pin") {
            ctx.beginPath()
            ctx.moveTo(0.30, 0.16)
            ctx.lineTo(0.70, 0.16)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.38, 0.16)
            ctx.lineTo(0.34, 0.56)
            ctx.lineTo(0.22, 0.66)
            ctx.lineTo(0.78, 0.66)
            ctx.lineTo(0.66, 0.56)
            ctx.lineTo(0.62, 0.16)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.50, 0.66)
            ctx.lineTo(0.50, 0.90)
            ctx.stroke()
        } else if (kind === "menu") {
            for (var m = 0; m < 3; ++m) {
                ctx.beginPath()
                ctx.moveTo(0.16, 0.26 + m * 0.24)
                ctx.lineTo(0.84, 0.26 + m * 0.24)
                ctx.lineWidth = strokeScale * 1.3
                ctx.stroke()
            }
            ctx.lineWidth = strokeScale
        } else if (kind === "undo" || kind === "redo") {
            var dir = kind === "undo" ? 1 : -1
            ctx.save()
            ctx.translate(0.5, 0.5)
            ctx.scale(dir, 1)
            ctx.beginPath()
            ctx.arc(0.0, 0.05, 0.30, Math.PI, 2 * Math.PI, false)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(-0.30, 0.05)
            ctx.lineTo(-0.30, 0.32)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(-0.30, 0.32)
            ctx.lineTo(-0.44, 0.16)
            ctx.moveTo(-0.30, 0.32)
            ctx.lineTo(-0.14, 0.20)
            ctx.stroke()
            ctx.restore()
        } else if (kind === "swatch") {
            ctx.beginPath()
            ctx.arc(0.50, 0.50, 0.34, 0, 2 * Math.PI, false)
            ctx.fill()
        } else if (kind === "bucket") {
            ctx.save()
            ctx.translate(0.46, 0.46)
            ctx.rotate(-0.5)
            ctx.beginPath()
            ctx.moveTo(-0.28, -0.20)
            ctx.lineTo(0.28, -0.20)
            ctx.lineTo(0.18, 0.28)
            ctx.lineTo(-0.18, 0.28)
            ctx.closePath()
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(-0.28, -0.20)
            ctx.bezierCurveTo(-0.28, -0.44, 0.28, -0.44, 0.28, -0.20)
            ctx.stroke()
            ctx.restore()
            ctx.beginPath()
            ctx.moveTo(0.82, 0.52)
            ctx.bezierCurveTo(0.94, 0.70, 0.90, 0.86, 0.80, 0.86)
            ctx.bezierCurveTo(0.70, 0.86, 0.68, 0.70, 0.82, 0.52)
            ctx.closePath()
            ctx.fill()
        } else if (kind === "pathops") {
            ctx.beginPath()
            ctx.moveTo(0.14, 0.72)
            ctx.bezierCurveTo(0.34, 0.16, 0.66, 0.16, 0.86, 0.72)
            ctx.stroke()
            ctx.beginPath()
            ctx.rect(0.06, 0.64, 0.16, 0.16)
            ctx.stroke()
            ctx.beginPath()
            ctx.rect(0.78, 0.64, 0.16, 0.16)
            ctx.stroke()
        } else if (kind === "unite") {
            ctx.beginPath()
            ctx.arc(0.38, 0.50, 0.28, 0, 2 * Math.PI, false)
            ctx.fill()
            ctx.beginPath()
            ctx.arc(0.62, 0.50, 0.28, 0, 2 * Math.PI, false)
            ctx.fill()
        } else if (kind === "subtract") {
            ctx.beginPath()
            ctx.arc(0.38, 0.50, 0.28, 0, 2 * Math.PI, false)
            ctx.fill()
            ctx.save()
            ctx.globalCompositeOperation = "destination-out"
            ctx.beginPath()
            ctx.arc(0.66, 0.50, 0.28, 0, 2 * Math.PI, false)
            ctx.fill()
            ctx.restore()
            ctx.beginPath()
            ctx.arc(0.66, 0.50, 0.28, 0, 2 * Math.PI, false)
            ctx.stroke()
        } else if (kind === "joinpaths") {
            ctx.beginPath()
            ctx.moveTo(0.10, 0.70)
            ctx.bezierCurveTo(0.26, 0.26, 0.40, 0.26, 0.48, 0.50)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.52, 0.50)
            ctx.bezierCurveTo(0.60, 0.74, 0.74, 0.74, 0.90, 0.30)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(0.50, 0.50, 0.09, 0, 2 * Math.PI, false)
            ctx.fill()
        } else if (kind === "breakapart") {
            ctx.beginPath()
            ctx.moveTo(0.10, 0.62)
            ctx.bezierCurveTo(0.22, 0.24, 0.34, 0.24, 0.40, 0.44)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.60, 0.56)
            ctx.bezierCurveTo(0.66, 0.76, 0.78, 0.76, 0.90, 0.38)
            ctx.stroke()
            ctx.beginPath()
            ctx.rect(0.32, 0.36, 0.14, 0.14)
            ctx.stroke()
            ctx.beginPath()
            ctx.rect(0.54, 0.50, 0.14, 0.14)
            ctx.stroke()
        } else if (kind === "textpath") {
            ctx.beginPath()
            ctx.moveTo(0.22, 0.76)
            ctx.lineTo(0.50, 0.20)
            ctx.lineTo(0.78, 0.76)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.34, 0.54)
            ctx.lineTo(0.66, 0.54)
            ctx.stroke()
            var tp = [ [0.22, 0.76], [0.50, 0.20], [0.78, 0.76] ]
            for (var ti = 0; ti < tp.length; ++ti) {
                ctx.beginPath()
                ctx.rect(tp[ti][0] - 0.07, tp[ti][1] - 0.07, 0.14, 0.14)
                ctx.stroke()
            }
        } else if (kind === "lasso") {
            ctx.save()
            ctx.setLineDash ? ctx.setLineDash([0.07, 0.05]) : 0
            ctx.beginPath()
            ctx.moveTo(0.50, 0.14)
            ctx.bezierCurveTo(0.90, 0.16, 0.92, 0.62, 0.58, 0.72)
            ctx.bezierCurveTo(0.26, 0.80, 0.06, 0.52, 0.22, 0.30)
            ctx.bezierCurveTo(0.30, 0.19, 0.40, 0.14, 0.50, 0.14)
            ctx.stroke()
            ctx.restore()
            ctx.beginPath()
            ctx.moveTo(0.34, 0.66)
            ctx.lineTo(0.28, 0.90)
            ctx.stroke()
        } else if (kind === "pickpoint") {
            ctx.beginPath()
            ctx.arc(0.50, 0.50, 0.30, 0, 2 * Math.PI, false)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(0.50, 0.50, 0.09, 0, 2 * Math.PI, false)
            ctx.fill()
            ctx.beginPath()
            ctx.moveTo(0.50, 0.06)
            ctx.lineTo(0.50, 0.20)
            ctx.moveTo(0.50, 0.80)
            ctx.lineTo(0.50, 0.94)
            ctx.moveTo(0.06, 0.50)
            ctx.lineTo(0.20, 0.50)
            ctx.moveTo(0.80, 0.50)
            ctx.lineTo(0.94, 0.50)
            ctx.stroke()
        } else if (kind === "nodes") {
            ctx.beginPath()
            ctx.moveTo(0.14, 0.72)
            ctx.bezierCurveTo(0.34, 0.20, 0.66, 0.20, 0.86, 0.72)
            ctx.stroke()
            var nodePts = [ [0.14, 0.72], [0.50, 0.38], [0.86, 0.72] ]
            for (var ni = 0; ni < nodePts.length; ++ni) {
                ctx.beginPath()
                ctx.rect(nodePts[ni][0] - 0.08, nodePts[ni][1] - 0.08, 0.16, 0.16)
                if (ni === 1)
                    ctx.fill()
                else
                    ctx.stroke()
            }
        } else if (kind === "transform") {
            ctx.beginPath()
            ctx.rect(0.20, 0.20, 0.60, 0.60)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.38, 0.50)
            ctx.lineTo(0.62, 0.50)
            ctx.moveTo(0.50, 0.38)
            ctx.lineTo(0.50, 0.62)
            ctx.stroke()
        } else if (kind === "resize") {
            ctx.beginPath()
            ctx.rect(0.16, 0.16, 0.52, 0.52)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.50, 0.86)
            ctx.lineTo(0.86, 0.86)
            ctx.lineTo(0.86, 0.50)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(0.58, 0.58)
            ctx.lineTo(0.86, 0.86)
            ctx.stroke()
        } else if (kind === "fliph" || kind === "flipv") {
            ctx.save()
            ctx.translate(0.5, 0.5)
            if (kind === "flipv")
                ctx.rotate(Math.PI / 2)
            ctx.beginPath()
            ctx.moveTo(0, -0.36)
            ctx.lineTo(0, 0.36)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(-0.10, -0.26)
            ctx.lineTo(-0.38, 0)
            ctx.lineTo(-0.10, 0.26)
            ctx.closePath()
            ctx.fill()
            ctx.beginPath()
            ctx.moveTo(0.10, -0.26)
            ctx.lineTo(0.38, 0)
            ctx.lineTo(0.10, 0.26)
            ctx.closePath()
            ctx.stroke()
            ctx.restore()
        }

        ctx.restore()
    }
}
