const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("audioControl", {
  startRecording: () => ipcRenderer.invoke("start-recording"),
  stopRecording: () => ipcRenderer.invoke("stop-recording"),
  getRecordingStatus: () => ipcRenderer.invoke("get-recording-status"),
});
