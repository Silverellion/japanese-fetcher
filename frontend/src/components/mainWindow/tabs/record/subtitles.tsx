import React, { useEffect, useState } from "react";
import { motion, AnimatePresence } from "framer-motion";

declare global {
  interface Window {
    fileSystem: {
      watchTranscripts: (callback: (text: string) => void) => () => void;
      onBackendReady: (callback: () => void) => () => void;
    };
  }
}

const Subtitles: React.FC = () => {
  const [subtitles, setSubtitles] = useState<string[]>([]);
  const [backendReady, setBackendReady] = useState(false);

  useEffect(() => {
    const cleanup = window.fileSystem.onBackendReady(() => {
      setBackendReady(true);
    });
    return cleanup;
  }, []);

  useEffect(() => {
    if (!backendReady) return;

    const handleNewTranscript = (text: string) => {
      const cleanedText = text.replace(/\r?\n/g, " ").replace(/\s+/g, " ").trim();
      setSubtitles((prev) => [...prev.slice(-2), cleanedText]);
    };

    const cleanup = window.fileSystem.watchTranscripts(handleNewTranscript);
    return cleanup;
  }, [backendReady]);

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
      <AnimatePresence>
        {subtitles.map((subtitle, index) => (
          <motion.div
            key={`${subtitle}-${index}`}
            initial={{ opacity: 0, y: 20 }}
            animate={{
              opacity: 1,
              y: (subtitles.length - 1 - index) * -40,
              color: `rgb(${255 - index * 55}, ${255 - index * 55}, ${255 - index * 55})`,
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
          >
            {subtitle}
          </motion.div>
        ))}
      </AnimatePresence>
    </div>
  );
};

export default Subtitles;
