import React from "react";
import { motion, AnimatePresence } from "framer-motion";

declare global {
  interface Window {
    fileSystem: {
      watchTranscripts: (callback: (text: string) => void) => () => void;
      onBackendReady: (callback: () => void) => () => void;
    };
    electronAPI?: {
      onMainWindowState: (callback: (state: "minimized" | "closed" | "restored") => void) => void;
    };
    kuroshiro: {
      convert: (text: string, options: any) => Promise<string>;
    };
    __stickyWindowSetVisible?: (visible: boolean) => void;
  }
}

const MAX_LINES = 2;

const StickyWindow: React.FC = () => {
  const [visible, setVisible] = React.useState(false);
  const [backendReady, setBackendReady] = React.useState(false);
  const [subtitles, setSubtitles] = React.useState<string[]>([]);
  const [furiganaSubtitles, setFuriganaSubtitles] = React.useState<string[]>([]);

  React.useEffect(() => {
    window.__stickyWindowSetVisible = setVisible;

    return () => {
      window.__stickyWindowSetVisible = undefined;
    };
  }, []);

  React.useEffect(() => {
    if (window.electronAPI && window.electronAPI.onMainWindowState) {
      window.electronAPI.onMainWindowState((state) => {
        setVisible(state === "minimized" || state === "closed");
      });
    } else {
      setVisible(true);
    }
  }, []);

  React.useEffect(() => {
    const cleanup = window.fileSystem.onBackendReady(() => {
      setBackendReady(true);
    });
    return cleanup;
  }, []);

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
  }, [backendReady]);

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

  if (!visible) return null;

  return (
    <div
      className="fixed left-0 bottom-0 w-full flex flex-col items-center pointer-events-none"
      style={{
        background: "rgba(30,30,30,0.7)",
        padding: "24px 0 12px 0",
        zIndex: 9999,
        minHeight: 120,
      }}
    >
      <AnimatePresence>
        {displaySubtitles.map((subtitle, index) => {
          const isLast = index === displaySubtitles.length - 1;
          const color = isLast ? "rgb(255,255,255)" : "rgb(180,180,180)";
          const fontSize = isLast ? 28 : 20;
          const y = isLast ? 0 : -36;
          return (
            <motion.div
              className="motion-div"
              key={`${subtitle}-${index}`}
              initial={{ opacity: 0, y: 20 }}
              animate={{
                opacity: 1,
                y,
                color,
                fontSize,
                fontWeight: isLast ? 700 : 400,
                textShadow: isLast ? "0 2px 8px rgba(0,0,0,0.7)" : "0 1px 4px rgba(0,0,0,0.5)",
              }}
              exit={{ opacity: 0 }}
              style={{
                margin: isLast ? "0 0 0 0" : "0 0 8px 0",
                whiteSpace: "pre-line",
                pointerEvents: "none",
                width: "100%",
                textAlign: "center",
              }}
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

export default StickyWindow;
