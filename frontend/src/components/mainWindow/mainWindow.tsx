import React from "react";
import Sidebar from "./sidebar/sidebar";
import TabSwitcher from "./sidebar/tabSwitcher";
import { type Tab } from "../../types";
import { type SubtitleState } from "../../App";

interface MainWindowProps extends SubtitleState {}

const MainWindow: React.FC<MainWindowProps> = ({
  subtitles,
  setSubtitles,
  backendReady,
  setBackendReady,
}) => {
  const [currentTab, setCurrentTab] = React.useState<Tab>("record");
  return (
    <>
      <div className="flex h-screen w-screen bg-[rgb(30,30,30)]">
        <Sidebar currentTab={currentTab} onTabChange={setCurrentTab}></Sidebar>
        <TabSwitcher
          currentTab={currentTab}
          subtitles={subtitles}
          setSubtitles={setSubtitles}
          backendReady={backendReady}
          setBackendReady={setBackendReady}
        />
      </div>
    </>
  );
};

export default MainWindow;
