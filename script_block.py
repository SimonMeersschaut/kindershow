import json
from pathlib import Path

import markdown
from markdown.extensions import Extension
from markdown.preprocessors import Preprocessor


CONFIG_PATH = Path(__file__).with_name("script_config.json")


def load_speaker_colors():
    try:
        with CONFIG_PATH.open("r", encoding="utf-8") as fh:
            data = json.load(fh)
        return data.get("speakers", {})
    except (FileNotFoundError, json.JSONDecodeError):
        return {}


class ScriptBlockPreprocessor(Preprocessor):
    def run(self, lines):
        new_lines = []
        i = 0

        while i < len(lines):
            line = lines[i].strip()

            if line.startswith("::: script"):
                block_lines = []
                i += 1

                while i < len(lines) and lines[i].strip() != ":::":
                    block_lines.append(lines[i])
                    i += 1

                if i < len(lines) and lines[i].strip() == ":::":
                    i += 1

                rendered = self._render_script(block_lines)
                if rendered:
                    new_lines.extend(rendered.splitlines())
                continue

            new_lines.append(lines[i])
            i += 1

        return new_lines

    def _render_script(self, lines):
        rendered_rows = []
        speaker_colors = load_speaker_colors()

        for raw_line in lines:
            text = raw_line.strip()
            if not text:
                continue

            if "::" in text:
                speaker, dialogue = text.split("::", 1)
            else:
                speaker, dialogue = "", text

            speaker = speaker.strip()
            dialogue = dialogue.strip()

            if not speaker and not dialogue:
                continue

            if not speaker:
                speaker = "<onbekend>"

            color = speaker_colors.get(speaker, speaker_colors.get("default", "#0f172a"))

            line_html = markdown.Markdown(extensions=["extra"]).convert(dialogue).strip()
            if line_html.startswith("<p>") and line_html.endswith("</p>"):
                line_html = line_html[3:-4]

            rendered_rows.append(
                f'<span class="show-script-speaker" style="color: {color};">{speaker}</span>'
            )
            rendered_rows.append(f'<span class="show-script-line">{line_html}</span><br/>')

        if not rendered_rows:
            return ""

        inner = "".join(rendered_rows)
        return (
            '<div class="show-script-wrapper">'
            f"{inner}"
            '</div>'
        )


class ScriptBlockExtension(Extension):
    def extendMarkdown(self, md):
        md.preprocessors.register(ScriptBlockPreprocessor(md), "script_block", 20)


def makeExtension():
    return ScriptBlockExtension()
