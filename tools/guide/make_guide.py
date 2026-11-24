#!/usr/bin/env python3
"""Builds docs/Pressura-Guide.pdf: an illustrated how-to in the plugin's palette and font (Spanish first, then English).

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

# Marker positions on the 388 x 508 panel (editor coordinates), number -> (x, y)
MARKERS = {1: (200, 14), 2: (18, 128), 3: (380, 156), 4: (60, 262), 5: (94, 262), 6: (196, 262),
           7: (293, 262), 8: (189, 378), 9: (376, 378), 10: (272, 94)}

TEXT = {
"en": dict(
    lang="en", subtitle="amplificator opticus · user guide",
    panel_title="The panel",
    legend=[
        ("vis", "Drive + saturation. Off: the red knob is <b>reductio</b>. On: it becomes <b>vis</b>, pushes the signal into the compressor and saturates it, dirtier the higher you turn it."),
        ("Display", "Shows the value while you turn a control; 2 s later, its name: <span class=led>RED</span> <span class=led>DRV</span> <span class=led>SPD</span> <span class=led>OUT</span> <span class=led>MIX</span>."),
        ("GR meter", "How many dB are being taken off right now (0–20). <span class=led>EXT</span> and <span class=led>VIS</span> light up when those modes are on."),
        ("mixtura", "Dry / wet. Pull it down for parallel compression (<i>siccus</i> = dry)."),
        ("reductio · vis", "How much it compresses (0–100). In vis mode: how hard you drive and saturate it."),
        ("impetus", "Speed. 50 = classic optical. Lower is smoother; higher is faster and punchier, and pumps."),
        ("exitus", "Output level, −18 to +18 dB."),
        ("compressio · limes", "Ratio 3:1 (gentle levelling) or 10:1 (limiting, squashing)."),
        ("alius", "Listen to another track: the one routed into Pressura's sidechain input in your DAW."),
        ("Presets", "‹ › step through 9 factory presets; click the name for the list. The display shows <span class=led>P01</span>–<span class=led>P09</span>; your DAW lists them too."),
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
        "Load the <b>Bass Sidechain</b> preset: it turns <b>alius</b> on (<span class=led>EXT</span>), so the drums duck the bass.",
        "Turn <b>reductio</b> until the bass dips 6–10 dB on each kick."],
    recipes_title="Presets", recipes_head=("Preset", "For"),
    recipes=[('Vocal Leveler', 'Vocals, voice-over: smooth optical levelling, about 3 dB.'), ('Bass Leveler', 'DI or synth bass: evens out the notes.'), ('Drum Bus Glue', 'Drum bus, 1176-style: kick and lows stay put, quieter percussion comes up ~3 dB.'), ('Mix Bus Glue', 'Mix bus: about 1–2 dB, SSL-style 2:1 glue, same level.'), ('Parallel Smash', 'Drums or anything: crushed ~11 dB at 10:1, blended in at 30 %.'), ('Peak Limiter', 'Anything: 10:1 and fast, only catches the peaks.'), ('Drum Bus Pump', 'Drum bus: vis drive and saturation, pumps like an all-buttons 1176.'), ('Crunch', 'Drums, loops, synths: vis saturation, dirty and dense.'), ('Bass Sidechain', "Bass ducked by the kick (~8 dB per hit). Route the drums to Pressura's sidechain.")],
    flow_title="Signal flow",
    flow=["Input", "vis drive", "trigger: own track or alius", "detector", "gain", "vis saturation", "mixtura", "exitus"],
    warn="With vis high and impetus low, transients can hit 0 dBFS: lower exitus or add a limiter after Pressura.",
    footer="Made by @gueibrielon · github.com/andesipa/pressura"),
"es": dict(
    lang="es", subtitle="amplificator opticus (amplificador óptico) · guía de uso",
    panel_title="El panel",
    legend=[
        ("vis <span class=tr>(fuerza)</span>", "Tecla de drive + saturación. Apagada: la perilla roja es <b>reductio</b>. Prendida: la perilla pasa a llamarse <b>vis</b>, empuja el sonido contra el compresor y lo ensucia; cuanto más la subís, más sucio."),
        ("Visor", "Mientras girás un control muestra su valor; 2 segundos después, su nombre: <span class=led>RED</span> reducción, <span class=led>DRV</span> drive, <span class=led>SPD</span> velocidad, <span class=led>OUT</span> salida, <span class=led>MIX</span> mezcla."),
        ("Medidor GR", "GR = reducción de ganancia: cuántos dB está bajando el compresor en este momento (0 a 20). <span class=led>EXT</span> se prende con alius y <span class=led>VIS</span> con vis."),
        ("mixtura <span class=tr>(mezcla)</span>", "Cuánto sonido comprimido se mezcla con el original. Arriba: todo comprimido. Abajo (<i>siccus</i> = seco): sin tocar. En el medio: compresión en paralelo."),
        ("reductio <span class=tr>(reducción)</span>", "Cuánto comprime, de 0 a 100. Con vis prendida: cuánto empuja y satura."),
        ("impetus <span class=tr>(ímpetu)</span>", "La velocidad del compresor. 50 = el óptico clásico. Más bajo: más suave y lento. Más alto: más rápido, con más golpe, y bombea con el ritmo."),
        ("exitus <span class=tr>(salida)</span>", "Volumen final, de −18 a +18 dB."),
        ("compressio · limes <span class=tr>(compresión · límite)</span>", "Cuánto aplasta lo que pasa el umbral: 3:1 para nivelar suave, 10:1 para limitar o aplastar."),
        ("alius <span class=tr>(otra fuente)</span>", "Pressura deja de escuchar su propia pista y escucha otra: la que mandás a su entrada de sidechain desde tu DAW."),
        ("Presets", "Ajustes listos para usar. ‹ › para pasar de uno a otro; click en el nombre para ver la lista. El visor muestra <span class=led>P01</span>–<span class=led>P09</span>; tu DAW también los muestra."),
    ],
    tip="Doble click en cualquier control para volverlo a su valor original.",
    use_title="Cómo usarlo",
    quick_title="Para empezar",
    quick=["Poné Pressura en una pista o en un bus (un grupo de pistas, por ejemplo la batería).",
           "Subí <b>reductio</b> hasta que el medidor GR marque 3 a 6 dB en las partes fuertes.",
           "Elegí la velocidad con <b>impetus</b>: 50 para un sonido suave, 80 o más para más golpe y bombeo.",
           "Igualá el volumen con <b>exitus</b>. Bajá <b>mixtura</b> para mezclar con el sonido original."],
    speed_title="impetus (velocidad)", speed_head=("", "ataque", "recuperación", "cómo suena"),
    speed_rows=[("0", "30 ms", "lenta", "nivela muy suave"),
                ("50", "10 ms", "60 ms + cola de 1 a 3 s", "el óptico clásico"),
                ("75", "1,7 ms", "40 ms", "apretado, agarra los golpes"),
                ("100", "0,3 ms", "30 ms", "rápido, bombea con el ritmo")],
    sc_title="Que el bombo haga bajar otra pista (sidechain)",
    sc=["En tu DAW, mandá la batería a la entrada de <b>sidechain</b> de Pressura. <i>Ableton:</i> sección Sidechain → Audio From. <i>Logic:</i> menú Side Chain. <i>FL Studio:</i> Sidechain to this track. <i>Reaper:</i> envío a los canales 3/4. <i>Cubase:</i> botón de side-chain.",
        "Cargá el preset <b>Bass Sidechain</b>: prende <b>alius</b> (en el visor se lee <span class=led>EXT</span>) y la batería hace bajar el bajo.",
        "Girá <b>reductio</b> hasta que el bajo baje 6 a 10 dB con cada bombo."],
    recipes_title="Presets de fábrica", recipes_head=("Nombre", "Para qué sirve"),
    recipes=[('Vocal Leveler', 'Voces y locución: empareja el volumen de forma suave, unos 3 dB.'), ('Bass Leveler', 'Bajo grabado directo o de sinte: empareja el volumen de las notas.'), ('Drum Bus Glue', 'Grupo de batería, al estilo de un 1176: el bombo y los graves quedan iguales y las percusiones de fondo suben unos 3 dB.'), ('Mix Bus Glue', 'Mezcla completa: 1 a 2 dB que "pegan" la mezcla, al estilo de un compresor SSL 2:1, sin cambiar el volumen.'), ('Parallel Smash', 'Batería o lo que sea: una copia muy aplastada (~11 dB a 10:1) mezclada al 30 % con el original.'), ('Peak Limiter', 'Cualquier pista: 10:1 y rápido, solo agarra los picos.'), ('Drum Bus Pump', 'Grupo de batería: drive y saturación vis, bombea con el ritmo como un 1176 con todos los botones apretados.'), ('Crunch', 'Batería, loops y sintes: saturación vis, sucio y denso.'), ('Bass Sidechain', 'El bombo hace bajar el bajo (~8 dB por golpe). Mandá la batería al sidechain de Pressura.')],
    flow_title="La señal",
    flow=["entrada", "drive vis", "detector", "compresión", "saturación vis", "mixtura", "exitus"],
    warn="Con vis alto e impetus bajo, los golpes pueden llegar a 0 dBFS (el máximo antes de distorsionar): bajá exitus o poné un limitador después de Pressura.",
    footer="Hecho por @gueibrielon · github.com/andesipa/pressura"),
}


def data_uri(path, mime):
    return f"data:{mime};base64,{base64.b64encode(path.read_bytes()).decode()}"


def title(text):
    # Red initial, like the plugin's title
    return f'<span class="initial">{text[0]}</span>{text[1:]}'


def pages(t):
    markers = "".join(f'<div class="marker" style="left:{x}px;top:{y}px">{n}</div>' for n, (x, y) in MARKERS.items())
    legend = "".join(f'<li><span class="num">{i}</span><div><span class="name">{name}</span> {desc}</div></li>'
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
    <div class="card body"><h3>{t["recipes_title"]}</h3><table class="recipes">{recipes}</table><p class="warn">{t["warn"]}</p></div>
  </div>
  <div class="flow"><span class="flow-title">{t["flow_title"]}</span>{flow}</div>
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
.panel-wrap { position: relative; width: 388px; height: 508px; flex: none; transform: scale(.94); transform-origin: top left;
              margin-right: -23px; margin-bottom: -30px; }
.panel { width: 388px; height: 508px; display: block; border-radius: 6px; box-shadow: 0 6px 18px rgba(0,0,0,.35); }
.marker { position: absolute; width: 22px; height: 22px; margin: -11px 0 0 -11px; border-radius: 50%;
          background: var(--red); border: 2px solid var(--key-cream); color: var(--key-cream);
          font: bold 12px/18px Georgia, serif; text-align: center; }
.legend-col { flex: 1; }
.legend { list-style: none; margin: 0; padding: 0; columns: 2; column-gap: 8mm; }
.legend li { display: flex; gap: 8px; break-inside: avoid; margin-bottom: 3.6mm; font-size: 13px; line-height: 1.4; }
.num { flex: none; width: 20px; height: 20px; border-radius: 50%; background: var(--red); color: var(--key-cream);
       font: bold 11px/20px Georgia, serif; text-align: center; margin-top: 1px; }
.name { font-family: Gothic, serif; font-size: 17px; }
.tr { font-family: Georgia, serif; font-style: italic; font-size: 12.5px; color: var(--ink-soft); }
.led { font: bold 10.5px/1 Menlo, monospace; color: var(--led); background: var(--screen); padding: 2px 4px; border-radius: 2px; letter-spacing: .5px; }
.tip { margin: 2mm 0 0; padding: 3mm 4mm; background: var(--panel-green); color: var(--key-cream); border-radius: 4px; font-size: 13px; }

/* Page 2: cards */
.grid { display: grid; grid-template-columns: 1fr 1fr; gap: 4mm 5mm; }
.grid > .card:nth-child(1) { grid-column: 1; grid-row: 1; }
.grid > .card:nth-child(2) { grid-column: 1; grid-row: 2; }
.grid > .card:nth-child(3) { grid-column: 1; grid-row: 3; }
.grid > .card:nth-child(4) { grid-column: 2; grid-row: 1 / span 3; }
.card { border-radius: 5px; padding: 3.2mm 5mm; font-size: 12.2px; line-height: 1.36; }
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
.recipes { font-size: 11.4px; }
.recipes td, .recipes th { border-bottom: 1px solid rgba(42,29,20,.18); }
.recipes td:first-child { white-space: nowrap; font-weight: bold; font-size: 11.4px; padding-right: 3mm; }
.recipes td { padding-top: .5mm; padding-bottom: .5mm; }
.flow { margin-top: 5mm; display: flex; flex-wrap: wrap; align-items: center; gap: 2mm; background: var(--screen);
        padding: 3mm 4mm; border-radius: 5px; }
.flow-title { font-family: Gothic, serif; font-size: 18px; color: var(--key-cream); margin-right: 3mm; }
.stage { font: bold 11px Menlo, monospace; color: var(--led); border: 1px solid rgba(232,50,30,.45); padding: 1.2mm 2mm; border-radius: 3px; }
.arrow { color: var(--key-cream); }
.warn { font-size: 11.5px; margin: 4mm 0 0; padding: 2.5mm 3mm; border-left: 3px solid var(--red); background: rgba(255,255,255,.12); }
"""

PANEL = data_uri(ROOT / "docs" / "pressura.png", "image/png")


def main():
    font = data_uri(ROOT / "Resources" / "UnifrakturMaguntia.ttf", "font/ttf")
    css = CSS.replace("FONT", font).replace("TOKENS", CSS_TOKENS)
    doc = f"""<!doctype html><html><head><meta charset="utf-8"><title>Pressura · guía de uso / user guide</title><style>{css}</style></head>
<body>{pages(TEXT["es"])}{pages(TEXT["en"])}</body></html>"""
    OUT_HTML.parent.mkdir(parents=True, exist_ok=True)
    OUT_HTML.write_text(doc, encoding="utf-8")

    if not pathlib.Path(CHROME).exists():
        sys.exit(f"Chrome not found at {CHROME}; open {OUT_HTML} and print it to PDF instead.")
    subprocess.run([CHROME, "--headless", "--disable-gpu", "--no-pdf-header-footer", "--virtual-time-budget=5000",
                    f"--print-to-pdf={OUT_PDF}", OUT_HTML.as_uri()], check=True, capture_output=True)
    print(f"Built {OUT_PDF.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
