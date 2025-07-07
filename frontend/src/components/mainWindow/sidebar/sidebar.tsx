import React from "react";
import { type Tab } from "../../../types";
import homeWhite from "../../../../assets/icons/home_white.svg";
import homeBlack from "../../../../assets/icons/home_black.svg";
import docsWhite from "../../../../assets/icons/docs_white.svg";
import docsBlack from "../../../../assets/icons/docs_black.svg";
import audioWhite from "../../../../assets/icons/audio_file_white.svg";
import audioBlack from "../../../../assets/icons/audio_file_black.svg";
import settingsWhite from "../../../../assets/icons/settings_white.svg";
import settingsBlack from "../../../../assets/icons/settings_black.svg";

interface SidebarProps {
  currentTab: Tab;
  onTabChange: (tab: Tab) => void;
}
const Sidebar: React.FC<SidebarProps> = ({ currentTab, onTabChange }) => {
  // prettier-ignore
  const tabs: {key: Tab, label: string, iconWhite: string, iconBlack: string}[] = [
    {key: 'home',        label: 'Home',        iconWhite: homeWhite,     iconBlack: homeBlack},
    {key: 'transcripts', label: 'Transcripts', iconWhite: docsWhite,     iconBlack: docsBlack},
    {key: 'audios',      label: 'Audios',      iconWhite: audioWhite,    iconBlack: audioBlack},
    {key: 'settings',    label: 'Settings',    iconWhite: settingsWhite, iconBlack: settingsBlack},
  ];
  return (
    <div className="bg-[rgb(15,15,15)]">
      {tabs.map((tab) => (
        <div
          key={tab.key}
          onClick={() => onTabChange(tab.key)}
          className={`flex gap-2 py-2 ps-2 pe-5 
            hover:bg-[rgb(0,0,0)] cursor-pointer
            ${currentTab === tab.key ? "bg-[rgb(0,0,0)]" : "bg-[rgb(15,15,15)]"}
          `}
        >
          <img src={tab.iconWhite} alt="" />
          <span className="text-white">{tab.label}</span>
        </div>
      ))}
    </div>
  );
};

export default Sidebar;
