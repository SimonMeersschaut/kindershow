import re

import markdown
from markdown.extensions import Extension
from markdown.preprocessors import Preprocessor


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
                speaker = "Narrator"

            line_html = markdown.Markdown(extensions=["extra"]).convert(dialogue).strip()
            if line_html.startswith("<p>") and line_html.endswith("</p>"):
                line_html = line_html[3:-4]

            rendered_rows.append(f'<p class="show-script-speaker">{speaker}</p>')
            rendered_rows.append(f'<p class="show-script-line">{line_html}</p>')

        if not rendered_rows:
            return ""

        inner = "".join(rendered_rows)
        return (
            '<div class="show-script-wrapper">'
            '<div class="show-script-scene">'
            f"{inner}"
            '</div>'
            '</div>'
        )


class ScriptBlockExtension(Extension):
    def extendMarkdown(self, md):
        md.preprocessors.register(ScriptBlockPreprocessor(md), "script_block", 20)


def makeExtension():
    return ScriptBlockExtension()
