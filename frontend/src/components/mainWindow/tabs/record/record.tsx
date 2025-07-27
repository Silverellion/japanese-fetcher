import React from "react";
import { Square, Circle } from "lucide-react";
import { type SubtitleState } from "../../../../App";
import Subtitles from "./subtitles";
import "./record.css";

declare global {
  interface Window {
    audioControl: {
      startRecording: () => Promise<boolean>;
      stopRecording: () => Promise<boolean>;
      getRecordingStatus: () => Promise<boolean>;
    };
  }
}

type RecordProps = Pick<
  SubtitleState,
  "subtitles" | "setSubtitles" | "backendReady" | "setBackendReady"
>;

const Record: React.FC<RecordProps> = ({
  subtitles,
  setSubtitles,
  backendReady,
  setBackendReady,
}) => {
  const [isRecording, setIsRecording] = React.useState(false);

  React.useEffect(() => {
    const checkRecordingStatus = async () => {
      const status = await window.audioControl.getRecordingStatus();
      setIsRecording(status);
    };

    checkRecordingStatus();
  }, []);

  const handleRecordToggle = async () => {
    if (isRecording) {
      await window.audioControl.stopRecording();
      setIsRecording(false);
    } else {
      await window.audioControl.startRecording();
      setIsRecording(true);
    }
  };

  return (
    <>
      <div className="flex flex-col h-full w-full">
        <div className="container h-full">
          <div className="flex justify-center items-center">
            <Subtitles
              subtitles={subtitles}
              setSubtitles={setSubtitles}
              backendReady={backendReady}
              setBackendReady={setBackendReady}
            />
          </div>
          <div className="flex justify-center items-center bg-[rgb(20,20,20)]">
            <button
              onClick={handleRecordToggle}
              className={`flex items-center justify-center cursor-pointer
              rounded-full w-12 h-12 transition-all duration-300
              hover:scale-110
              ${
                isRecording
                  ? "bg-[rgb(40,40,40)] hover:bg-[rgb(30,30,30)]"
                  : "bg-[rgb(200,50,50)] hover:bg-[rgb(200,0,0)]"
              }
              `}
            >
              {isRecording ? (
                <Square size={24} fill="white" className="text-white" />
              ) : (
                <Circle size={24} fill="white" className="text-white" />
              )}
            </button>
          </div>
        </div>
      </div>
    </>
  );
};

export default Record;
