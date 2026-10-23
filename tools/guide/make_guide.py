#!/usr/bin/env python3
"""Builds docs/Pressura-Guide.pdf: an illustrated how-to in the plugin's palette and font (English + Spanish).

    python3 tools/guide/make_guide.py

Needs Google Chrome (used headless to print the HTML to PDF). Uses docs/pressura.png for the panel picture;
regenerate that first with the snapshot tool if the UI changed.
"""
import base64, html, pathlib, subprocess, sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
OUT_HTML = ROOT / "build" / "guide" / "guide.html"
OUT_PDF = ROOT / "docs" / "Pressura-Guide.pdf"
CHROME = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"

# Same colours as Palette in Source/PluginEditor.h
CSS_TOKENS = """
  --body-light:#a38e57; --body:#9a8550; --body-dark:#8c7841;
  --ink:#2a1d14; --ink-soft:#5e4a2a;
  --panel-green:#2c3530; --key-green:#1c2622; --key-brown:#3c2521; --key-cream:#e0d2c3;
  --red:#bf0e06; --key-red:#9e150a; --screen:#191413; --led:#e8321e; --speaker:#332015;
"""

# Marker positions on the 388 x 626 panel (editor coordinates), number -> (x, y)
MARKERS = {1: (200, 14), 2: (18, 128), 3: (380, 156), 4: (60, 262), 5: (94, 262), 6: (196, 262),
           7: (293, 262), 8: (238, 378), 9: (6, 506), 10: (96, 500)}

TEXT = {
"en": dict(
    lang="en", subtitle="amplificator opticus · user guide",
    panel_title="The panel",
    legend=[
        ("vis", "Drive mode. Off: the red knob is <b>reductio</b>. On: it becomes <b>vis</b> and pushes the signal into the compressor, denser and louder."),
        ("Display", "Shows the value while you turn a control; 2 s later, its name: <span class=led>RED</span> <span class=led>DRV</span> <span class=led>SPD</span> <span class=led>OUT</span> <span class=led>MIX</span> <span class=led>LO</span> <span class=led>MID</span> <span class=led>HI</span>."),
        ("GR meter", "How many dB are being taken off right now (0–20). <span class=led>EXT</span> and <span class=led>VIS</span> light up when those modes are on."),
        ("mixtura", "Dry / wet. Pull it down for parallel compression (<i>siccus</i> = dry)."),
        ("reductio · vis", "How much it compresses (0–100). In vis mode: how hard you drive it."),
        ("impetus", "Speed. 50 = classic optical. Lower is smoother; higher is faster and punchier, and pumps."),
        ("exitus", "Output level, −18 to +18 dB."),
        ("compressio · limes", "Ratio 3:1 (gentle levelling) or 10:1 (limiting, squashing)."),
        ("alius", "Listen to another track: the one routed into Pressura's sidechain input in your DAW."),
        ("gravis · medius · acutus", "Trigger EQ: how much the lows (40–200 Hz), mids (200 Hz–1 kHz) and highs (1–20 kHz) make it compress. Straight up = 0 dB, all the way down = <span class=led>OFF</span>."),
    ],
    tip="Double-click any control to reset it.",
    use_title="How to use it",
    quick_title="Quick start",
    quick=["Put Pressura on a track or a bus.",
           "Turn <b>reductio</b> up until the GR meter moves 3–6 dB on the loud parts.",
           "Set <b>impetus</b>: 50 for smooth, 80 and up for punch and pump.",
           "Match the level with <b>exitus</b>. Lower <b>mixtura</b> for parallel compression."],
    speed_title="impetus", speed_head=("", "attack", "release", "sound"),
    speed_rows=[("0", "30 ms", "slow", "very smooth levelling"),
                ("50", "10 ms", "60 ms + 1–3 s tail", "classic optical"),
                ("75", "1.7 ms", "40 ms", "tight, catches transients"),
                ("100", "0.3 ms", "30 ms", "fast, pumps with the beat")],
    sc_title="Duck a track with the kick",
    sc=["Route the drums into Pressura's <b>sidechain</b> in your DAW. <i>Ableton:</i> Sidechain section → Audio From. <i>Logic:</i> Side Chain menu. <i>FL Studio:</i> Sidechain to this track. <i>Reaper:</i> send to channels 3/4. <i>Cubase:</i> side-chain button.",
        "Press <b>alius</b> (<span class=led>EXT</span> lights up).",
        "<b>gravis</b> +6 dB, <b>medius</b> and <b>acutus</b> OFF: only the kick triggers it.",
        "<b>reductio</b> 50–70, <b>impetus</b> 75–90."],
    recipes_title="Starting points", recipes_head=("Use", "Red knob", "impetus", "Ratio", "Also"),
    recipes=[("Vocal or bass levelling", "reductio 40–60", "40–50", "compressio", "—"),
             ("Mix bus glue", "reductio 15–30", "30–50", "compressio", "—"),
             ("Drum bus that pumps", "vis 50–70", "80–100", "compressio", "vis on · mixtura 60–80 %"),
             ("Parallel smash", "reductio 80–100", "70–90", "limes", "mixtura 20–40 %"),
             ("Loud and dense", "vis 60–90", "50–70", "limes", "vis on · lower exitus"),
             ("Bass ducked by the kick", "reductio 50–70", "75–90", "compressio", "alius · gravis +6 · medius, acutus OFF"),
             ("Bus that doesn't pump on bass", "reductio 20–30", "40–50", "compressio", "gravis −12")],
    flow_title="Signal flow",
    flow=["Input", "vis drive", "trigger: own track or alius", "trigger EQ", "detector", "gain", "mixtura", "exitus"],
    warn="With vis high and impetus low, transients can hit 0 dBFS: lower exitus or add a limiter after Pressura.",
    footer="Made by @gueibrielon · github.com/andesipa/pressura"),
"es": dict(
    lang="es", subtitle="amplificator opticus · guía de uso",
    panel_title="El panel",
    legend=[
        ("vis", "Modo drive. Apagada: la perilla roja es <b>reductio</b>. Prendida: pasa a <b>vis</b> y empuja la señal contra el compresor, más denso y más fuerte."),
        ("Visor", "Muestra el valor mientras girás un control; 2 s después, el nombre: <span class=led>RED</span> <span class=led>DRV</span> <span class=led>SPD</span> <span class=led>OUT</span> <span class=led>MIX</span> <span class=led>LO</span> <span class=led>MID</span> <span class=led>HI</span>."),
        ("Medidor GR", "Cuántos dB se están bajando en este momento (0–20). <span class=led>EXT</span> y <span class=led>VIS</span> se prenden con esos modos."),
        ("mixtura", "Seco / procesado. Bajalo para compresión en paralelo (<i>siccus</i> = seco)."),
        ("reductio · vis", "Cuánto comprime (0–100). En modo vis: cuánto lo empujás."),
        ("impetus", "Velocidad. 50 = el óptico clásico. Más bajo, más suave; más alto, más rápido y con más golpe, y bombea."),
        ("exitus", "Nivel de salida, −18 a +18 dB."),
        ("compressio · limes", "Ratio 3:1 (nivelar suave) o 10:1 (limitar, aplastar)."),
        ("alius", "Escuchar otra pista: la que ruteás a la entrada de sidechain de Pressura en tu DAW."),
        ("gravis · medius · acutus", "EQ del disparo: cuánto hacen comprimir los graves (40–200 Hz), medios (200 Hz–1 kHz) y agudos (1–20 kHz). Derecha = 0 dB, todo abajo = <span class=led>OFF</span>."),
    ],
    tip="Doble click en cualquier control para volverlo a su valor por defecto.",
    use_title="Cómo usarlo",
    quick_title="Para empezar",
    quick=["Poné Pressura en una pista o en un bus.",
           "Subí <b>reductio</b> hasta que el medidor GR se mueva 3–6 dB en las partes fuertes.",
           "Ajustá <b>impetus</b>: 50 para suave, 80 o más para golpe y bombeo.",
           "Igualá el volumen con <b>exitus</b>. Bajá <b>mixtura</b> para compresión en paralelo."],
    speed_title="impetus", speed_head=("", "ataque", "release", "sonido"),
    speed_rows=[("0", "30 ms", "lento", "nivelación muy suave"),
                ("50", "10 ms", "60 ms + cola de 1–3 s", "óptico clásico"),
                ("75", "1.7 ms", "40 ms", "apretado, agarra transitorios"),
                ("100", "0.3 ms", "30 ms", "rápido, bombea con el beat")],
    sc_title="Que el bombo agache una pista",
    sc=["Ruteá la batería al <b>sidechain</b> de Pressura en tu DAW. <i>Ableton:</i> sección Sidechain → Audio From. <i>Logic:</i> menú Side Chain. <i>FL Studio:</i> Sidechain to this track. <i>Reaper:</i> send a los canales 3/4. <i>Cubase:</i> botón de side-chain.",
        "Prendé <b>alius</b> (se prende <span class=led>EXT</span>).",
        "<b>gravis</b> +6 dB, <b>medius</b> y <b>acutus</b> en OFF: solo el bombo dispara.",
        "<b>reductio</b> 50–70, <b>impetus</b> 75–90."],
    recipes_title="Puntos de partida", recipes_head=("Uso", "Perilla roja", "impetus", "Ratio", "Además"),
    recipes=[("Nivelar voz o bajo", "reductio 40–60", "40–50", "compressio", "—"),
             ("Pegar el bus master", "reductio 15–30", "30–50", "compressio", "—"),
             ("Bus de batería que bombea", "vis 50–70", "80–100", "compressio", "vis prendida · mixtura 60–80 %"),
             ("Aplastado en paralelo", "reductio 80–100", "70–90", "limes", "mixtura 20–40 %"),
             ("Fuerte y denso", "vis 60–90", "50–70", "limes", "vis prendida · bajá exitus"),
             ("Bajo que se agacha con el bombo", "reductio 50–70", "75–90", "compressio", "alius · gravis +6 · medius, acutus OFF"),
             ("Bus que no bombea con el grave", "reductio 20–30", "40–50", "compressio", "gravis −12")],
    flow_title="Recorrido de la señal",
    flow=["Entrada", "drive vis", "disparo: propia o alius", "EQ del disparo", "detector", "ganancia", "mixtura", "exitus"],
    warn="Con vis alto e impetus bajo, los transitorios pueden llegar a 0 dBFS: bajá exitus o poné un limitador después de Pressura.",
    footer="Hecho por @gueibrielon · github.com/andesipa/pressura"),
}


def data_uri(path, mime):
    return f"data:{mime};base64,{base64.b64encode(path.read_bytes()).decode()}"


def title(text):
    # Red initial, like the plugin's title
    return f'<span class="initial">{text[0]}</span>{text[1:]}'


def pages(t):
    markers = "".join(f'<div class="marker" style="left:{x}px;top:{y}px">{n}</div>' for n, (x, y) in MARKERS.items())
    legend = "".join(f'<li><span class="num">{i}</span><div><span class="name">{html.escape(name)}</span> {desc}</div></li>'
                     for i, (name, desc) in enumerate(t["legend"], 1))
    quick = "".join(f"<li>{s}</li>" for s in t["quick"])
    sc = "".join(f"<li>{s}</li>" for s in t["sc"])
    speed = "<tr>" + "".join(f"<th>{h}</th>" for h in t["speed_head"]) + "</tr>" + "".join(
        "<tr>" + f'<td class="led-cell">{r[0]}</td>' + "".join(f"<td>{c}</td>" for c in r[1:]) + "</tr>" for r in t["speed_rows"])
    recipes = "<tr>" + "".join(f"<th>{h}</th>" for h in t["recipes_head"]) + "</tr>" + "".join(
        "<tr>" + "".join(f"<td>{c}</td>" for c in r) + "</tr>" for r in t["recipes"])
    flow = '<span class="arrow">→</span>'.join(f'<span class="stage">{s}</span>' for s in t["flow"])

    return f"""
<section class="page">
  <header><h1>{title("Pressura")}</h1><p class="sub">{t["subtitle"]}</p></header>
  <div class="spread">
    <div class="panel-wrap"><img class="panel" src="{PANEL}" alt="">{markers}</div>
    <div class="legend-col">
      <h2>{title(t["panel_title"])}</h2>
      <ol class="legend">{legend}</ol>
      <p class="tip">{t["tip"]}</p>
    </div>
  </div>
  <footer>{t["footer"]}</footer>
</section>
<section class="page">
  <header><h1>{title(t["use_title"])}</h1><p class="sub">{t["subtitle"]}</p></header>
  <div class="grid">
    <div class="card green"><h3>{t["quick_title"]}</h3><ol class="steps">{quick}</ol></div>
    <div class="card screen"><h3>{t["speed_title"]}</h3><table class="speed">{speed}</table></div>
    <div class="card green wide-left"><h3>{t["sc_title"]}</h3><ol class="steps">{sc}</ol></div>
    <div class="card body"><h3>{t["recipes_title"]}</h3><table class="recipes">{recipes}</table></div>
  </div>
  <div class="flow"><span class="flow-title">{t["flow_title"]}</span>{flow}</div>
  <p class="warn">{t["warn"]}</p>
  <footer>{t["footer"]}</footer>
</section>"""


CSS = """
@font-face { font-family: Gothic; src: url("FONT") format("truetype"); }
@page { size: A4 landscape; margin: 0; }
:root { TOKENS }
* { box-sizing: border-box; -webkit-print-color-adjust: exact; print-color-adjust: exact; }
body { margin: 0; font-family: Georgia, "Iowan Old Style", serif; color: var(--ink); background: var(--body); }
.page { width: 297mm; height: 210mm; padding: 9mm 12mm 8mm; position: relative; overflow: hidden;
        background: linear-gradient(var(--body-light), var(--body-dark)); page-break-after: always; break-after: page; }
header { display: flex; align-items: baseline; gap: 14px; border-bottom: 1px solid rgba(42,29,20,.25); padding-bottom: 2mm; margin-bottom: 4mm; }
h1, h2, h3 { font-family: Gothic, serif; font-weight: normal; margin: 0; }
h1 { font-size: 40px; line-height: 1; }
h2 { font-size: 28px; margin-bottom: 3mm; }
h3 { font-size: 22px; margin-bottom: 2mm; }
.initial { color: var(--red); }
.sub { font-family: Gothic, serif; font-size: 18px; margin: 0; color: var(--ink-soft); }
footer { position: absolute; left: 12mm; right: 12mm; bottom: 5mm; font-family: Gothic, serif; font-size: 14px;
         border-top: 1px solid rgba(42,29,20,.25); padding-top: 1.5mm; }

/* Page 1: panel + legend */
.spread { display: flex; gap: 10mm; }
.panel-wrap { position: relative; width: 388px; height: 626px; flex: none; transform: scale(.94); transform-origin: top left;
              margin-right: -23px; margin-bottom: -38px; }
.panel { width: 388px; height: 626px; display: block; border-radius: 6px; box-shadow: 0 6px 18px rgba(0,0,0,.35); }
.marker { position: absolute; width: 22px; height: 22px; margin: -11px 0 0 -11px; border-radius: 50%;
          background: var(--red); border: 2px solid var(--key-cream); color: var(--key-cream);
          font: bold 12px/18px Georgia, serif; text-align: center; }
.legend-col { flex: 1; }
.legend { list-style: none; margin: 0; padding: 0; columns: 2; column-gap: 8mm; }
.legend li { display: flex; gap: 8px; break-inside: avoid; margin-bottom: 3.6mm; font-size: 13px; line-height: 1.4; }
.num { flex: none; width: 20px; height: 20px; border-radius: 50%; background: var(--red); color: var(--key-cream);
       font: bold 11px/20px Georgia, serif; text-align: center; margin-top: 1px; }
.name { font-family: Gothic, serif; font-size: 17px; }
.led { font: bold 10.5px/1 Menlo, monospace; color: var(--led); background: var(--screen); padding: 2px 4px; border-radius: 2px; letter-spacing: .5px; }
.tip { margin: 2mm 0 0; padding: 3mm 4mm; background: var(--panel-green); color: var(--key-cream); border-radius: 4px; font-size: 13px; }

/* Page 2: cards */
.grid { display: grid; grid-template-columns: 1fr 1fr; gap: 5mm; }
.card { border-radius: 5px; padding: 4mm 5mm; font-size: 12.5px; line-height: 1.4; }
.card.green { background: var(--panel-green); color: var(--key-cream); }
.card.screen { background: var(--screen); color: var(--key-cream); }
.card.screen h3 { color: var(--led); }
.card.body { background: rgba(255,255,255,.12); border: 1px solid rgba(42,29,20,.25); }
.steps { margin: 0; padding-left: 5mm; }
.steps li { margin-bottom: 1.6mm; }
table { border-collapse: collapse; width: 100%; }
th { font-family: Gothic, serif; font-weight: normal; font-size: 15px; text-align: left; padding: 0 2mm 1mm 0; }
td { padding: 1mm 2mm 1mm 0; vertical-align: top; }
.speed td, .speed th { border-bottom: 1px solid rgba(232,50,30,.18); }
.speed .led-cell { font: bold 15px Menlo, monospace; color: var(--led); width: 12mm; }
.recipes { font-size: 11.2px; }
.recipes td, .recipes th { border-bottom: 1px solid rgba(42,29,20,.18); }
.recipes td:nth-child(2), .recipes td:nth-child(3), .recipes td:nth-child(4) { white-space: nowrap; }
.flow { margin-top: 5mm; display: flex; flex-wrap: wrap; align-items: center; gap: 2mm; background: var(--screen);
        padding: 3mm 4mm; border-radius: 5px; }
.flow-title { font-family: Gothic, serif; font-size: 18px; color: var(--key-cream); margin-right: 3mm; }
.stage { font: bold 11px Menlo, monospace; color: var(--led); border: 1px solid rgba(232,50,30,.45); padding: 1.2mm 2mm; border-radius: 3px; }
.arrow { color: var(--key-cream); }
.warn { font-size: 11.5px; margin: 3mm 0 0; color: var(--ink-soft); }
"""

PANEL = data_uri(ROOT / "docs" / "pressura.png", "image/png")


def main():
    font = data_uri(ROOT / "Resources" / "UnifrakturMaguntia.ttf", "font/ttf")
    css = CSS.replace("FONT", font).replace("TOKENS", CSS_TOKENS)
    doc = f"""<!doctype html><html><head><meta charset="utf-8"><title>Pressura guide</title><style>{css}</style></head>
<body>{pages(TEXT["en"])}{pages(TEXT["es"])}</body></html>"""
    OUT_HTML.parent.mkdir(parents=True, exist_ok=True)
    OUT_HTML.write_text(doc, encoding="utf-8")

    if not pathlib.Path(CHROME).exists():
        sys.exit(f"Chrome not found at {CHROME}; open {OUT_HTML} and print it to PDF instead.")
    subprocess.run([CHROME, "--headless", "--disable-gpu", "--no-pdf-header-footer", "--virtual-time-budget=5000",
                    f"--print-to-pdf={OUT_PDF}", OUT_HTML.as_uri()], check=True, capture_output=True)
    print(f"Built {OUT_PDF.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
