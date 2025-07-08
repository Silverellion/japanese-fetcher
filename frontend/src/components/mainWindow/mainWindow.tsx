import React from "react";
import Sidebar from "./sidebar/sidebar";
import TabSwitcher from "./sidebar/tabSwitcher";
import { type Tab } from "../../types";

const MainWindow: React.FC = () => {
  const [currentTab, setCurrentTab] = React.useState<Tab>("record");
  return (
    <>
      <div className="flex h-screen w-screen bg-[rgb(30,30,30)]">
        <Sidebar currentTab={currentTab} onTabChange={setCurrentTab}></Sidebar>
        <TabSwitcher currentTab={currentTab}></TabSwitcher>
      </div>
    </>
  );
};

export default MainWindow;
