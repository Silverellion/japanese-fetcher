import React from "react";
import { motion, AnimatePresence } from "framer-motion";
import { type SubtitleState } from "../../../../App";
import "./subtitles.css";

declare global {
  interface Window {
    fileSystem: {
      watchTranscripts: (callback: (text: string) => void) => () => void;
      onBackendReady: (callback: () => void) => () => void;
    };
    kuroshiro: {
      convert: (text: string, options: any) => Promise<string>;
    };
  }
}

const MAX_LINES = 3;

type SubtitlesProps = Pick<
  SubtitleState,
  "subtitles" | "setSubtitles" | "backendReady" | "setBackendReady"
>;

const Subtitles: React.FC<SubtitlesProps> = ({
  subtitles,
  setSubtitles,
  backendReady,
  setBackendReady,
}) => {
  const [furiganaSubtitles, setFuriganaSubtitles] = React.useState<string[]>([]);

  React.useEffect(() => {
    const cleanup = window.fileSystem.onBackendReady(() => {
      setBackendReady(true);
    });
    return cleanup;
  }, [setBackendReady]);

  React.useEffect(() => {
    if (!backendReady) return;

    let lastSubtitle = "";
    const handleNewTranscript = (text: string) => {
      if (text && text !== lastSubtitle) {
        setSubtitles((prev) => {
          const newArr = [...prev, text];
          return newArr.slice(-MAX_LINES);
        });
        lastSubtitle = text;
      }
    };

    const cleanup = window.fileSystem.watchTranscripts(handleNewTranscript);
    return cleanup;
  }, [backendReady, setSubtitles]);

  React.useEffect(() => {
    const processFurigana = async () => {
      if (subtitles.length === 0) return;

      const processed = await Promise.all(
        subtitles.map((text) =>
          window.kuroshiro.convert(text, {
            to: "hiragana",
            mode: "furigana",
            romajiSystem: "nippon",
          })
        )
      );

      setFuriganaSubtitles(processed);
    };

    processFurigana();
  }, [subtitles]);

  const processFuriganaHtml = (html: string) => {
    return html
      .replace(/<\/ruby>\s+<ruby>/g, "</ruby><ruby>")
      .replace(/(<\/ruby>)(\s+)([^\s<])/g, "$1$3");
  };

  const displaySubtitles = furiganaSubtitles.length > 0 ? furiganaSubtitles : subtitles;

  return (
    <div className="subtitles-container">
      <AnimatePresence>
        {displaySubtitles.map((subtitle, index) => {
          const age = displaySubtitles.length - 1 - index;
          const colorValue = 255 - age * 55;
          return (
            <motion.div
              className="motion-div"
              key={`${subtitle}-${index}`}
              initial={{ opacity: 0, y: 20 }}
              animate={{
                opacity: 1,
                y: age * -40,
                color: `rgb(${colorValue}, ${colorValue}, ${colorValue})`,
              }}
              exit={{ opacity: 0 }}
              {...(furiganaSubtitles.includes(subtitle)
                ? { dangerouslySetInnerHTML: { __html: processFuriganaHtml(subtitle) } }
                : { children: subtitle })}
            />
          );
        })}
      </AnimatePresence>
    </div>
  );
};

export default Subtitles;
