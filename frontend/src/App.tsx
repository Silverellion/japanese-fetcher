import "./global.css";
import MainWindow from "./components/mainWindow/mainWindow";
import React from "react";

export interface SubtitleState {
  subtitles: string[];
  setSubtitles: React.Dispatch<React.SetStateAction<string[]>>;
  backendReady: boolean;
  setBackendReady: React.Dispatch<React.SetStateAction<boolean>>;
}

function App() {
  const [subtitles, setSubtitles] = React.useState<string[]>([]);
  const [backendReady, setBackendReady] = React.useState(false);

  return (
    <>
      <div className="flex h-screen w-screen min-h-screen">
        <MainWindow
          subtitles={subtitles}
          setSubtitles={setSubtitles}
          backendReady={backendReady}
          setBackendReady={setBackendReady}
        />
      </div>
    </>
  );
}

export default App;
