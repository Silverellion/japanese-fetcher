import React from "react";
import { motion, AnimatePresence } from "framer-motion";

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

const Subtitles: React.FC = () => {
  const [subtitles, setSubtitles] = React.useState<string[]>([]);
  const [backendReady, setBackendReady] = React.useState(false);
  const [furiganaSubtitles, setFuriganaSubtitles] = React.useState<string[]>([]);
  const [processingError, setProcessingError] = React.useState<string | null>(null);

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
      setProcessingError(null);
    };

    processFurigana();
  }, [subtitles]);

  const displaySubtitles = furiganaSubtitles.length > 0 ? furiganaSubtitles : subtitles;

  return (
    <div
      style={{
        padding: "20px",
        position: "relative",
        height: "200px",
        width: "100%",
        display: "flex",
        flexDirection: "column",
        justifyContent: "flex-end",
        alignItems: "center",
      }}
    >
      {processingError && (
        <div style={{ color: "red", marginBottom: "10px", fontSize: "12px" }}>
          {processingError}
        </div>
      )}
      <AnimatePresence>
        {displaySubtitles.map((subtitle, index) => (
          <motion.div
            key={`${subtitle}-${index}`}
            initial={{ opacity: 0, y: 20 }}
            animate={{
              opacity: 1,
              y: (MAX_LINES - 1 - index) * -40,
              color: `rgb(${255 - (MAX_LINES - 1 - index) * 55}, ${
                255 - (MAX_LINES - 1 - index) * 55
              }, ${255 - (MAX_LINES - 1 - index) * 55})`,
            }}
            exit={{ opacity: 0 }}
            style={{
              position: "relative",
              width: "auto",
              textAlign: "center",
              fontSize: "18px",
              lineHeight: "1.5",
              margin: "4px 0",
              whiteSpace: "pre-line",
            }}
            {...(furiganaSubtitles.includes(subtitle)
              ? { dangerouslySetInnerHTML: { __html: subtitle } }
              : { children: subtitle })}
          />
        ))}
      </AnimatePresence>
    </div>
  );
};

export default Subtitles;
