import React from "react";
import Sidebar from "./sidebar/sidebar";
import { type Tab } from "../../types";

const MainWindow: React.FC = () => {
  const [currentTab, setCurrentTab] = React.useState<Tab>("home");
  return (
    <>
      <div className="flex h-screen w-screen bg-[rgb(30,30,30)]">
        <Sidebar currentTab={currentTab} onTabChange={setCurrentTab}></Sidebar>
      </div>
    </>
  );
};

export default MainWindow;
