import React, { useEffect } from "react";
import "./record.css";
import { Square, Circle } from "lucide-react";

declare global {
  interface Window {
    audioControl: {
      startRecording: () => Promise<boolean>;
      stopRecording: () => Promise<boolean>;
      getRecordingStatus: () => Promise<boolean>;
    };
  }
}

const Record: React.FC = () => {
  const [isRecording, setIsRecording] = React.useState(false);

  useEffect(() => {
    const checkRecordingStatus = async () => {
      try {
        const status = await window.audioControl.getRecordingStatus();
        setIsRecording(status);
      } catch (error) {
        console.error("Failed to get recording status:", error);
      }
    };

    checkRecordingStatus();
  }, []);

  const handleRecordToggle = async () => {
    try {
      if (isRecording) {
        await window.audioControl.stopRecording();
        setIsRecording(false);
      } else {
        await window.audioControl.startRecording();
        setIsRecording(true);
      }
    } catch (error) {
      console.error("Failed to toggle recording:", error);
    }
  };

  return (
    <>
      <div className="flex flex-col h-full w-full">
        <div className="container h-full">
          <div className="flex justify-center items-center">waveform graph</div>
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
