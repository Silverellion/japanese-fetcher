const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("audioControl", {
  startRecording: () => ipcRenderer.invoke("start-recording"),
  stopRecording: () => ipcRenderer.invoke("stop-recording"),
  getRecordingStatus: () => ipcRenderer.invoke("get-recording-status"),
});

contextBridge.exposeInMainWorld("fileSystem", {
  watchTranscripts: (callback) => {
    const handleTranscript = (_, text) => callback(text);
    ipcRenderer.on("new-transcript", handleTranscript);
    return () => ipcRenderer.removeListener("new-transcript", handleTranscript);
  },
  onBackendReady: (callback) => {
    const handleReady = () => callback();
    ipcRenderer.on("backend-ready", handleReady);
    return () => ipcRenderer.removeListener("backend-ready", handleReady);
  },
});

contextBridge.exposeInMainWorld("kuroshiro", {
  convert: (text, options) => ipcRenderer.invoke("convert-text", text, options),
});

ipcRenderer.on("main-window-state", (_, state) => {
  if (window.__stickyWindowSetVisible) {
    window.__stickyWindowSetVisible(state === "minimized" || state === "closed");
  }
});
