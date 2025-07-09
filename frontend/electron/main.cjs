const { app, BrowserWindow, globalShortcut, ipcMain } = require("electron");
const path = require("path");
const { exec } = require("child_process");
const isDev = process.env.NODE_ENV !== "production";

process.env.ELECTRON_DISABLE_SECURITY_WARNINGS = "true";
app.commandLine.appendSwitch("disable-features", "Autofill");

const backendPath = path.join(__dirname, "../../backend/cpp/x64/Release/cpp.exe");
let backendProcess = null;

function createWindow() {
  const mainWindow = new BrowserWindow({
    width: 1280,
    height: 720,
    minWidth: 960,
    minHeight: 540,
    show: false,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, "preload.cjs"),
    },
  });

  mainWindow.once("ready-to-show", () => {
    mainWindow.show();
  });

  if (isDev) {
    mainWindow.loadURL("http://localhost:5173");
    mainWindow.webContents.openDevTools();
    mainWindow.webContents.session.clearCache();
  } else {
    mainWindow.loadFile(path.join(__dirname, "../dist/index.html"));
  }

  globalShortcut.register("Control+F", () => {
    if (mainWindow.isFullScreen()) {
      mainWindow.setFullScreen(false);
    } else {
      mainWindow.setFullScreen(true);
    }
  });

  globalShortcut.register("Alt+Enter", () => {
    if (mainWindow.isFullScreen()) {
      mainWindow.setFullScreen(false);
    } else {
      mainWindow.setFullScreen(true);
    }
  });
}

// Handle IPC events for controlling the backend
ipcMain.handle("start-recording", async () => {
  if (!backendProcess) {
    backendProcess = exec(`"${backendPath}" --start-recording`, (error) => {
      if (error) {
        console.error(`Error starting recording: ${error}`);
        return false;
      }
    });
  } else {
    exec(`"${backendPath}" --command start-recording`);
  }
  return true;
});

ipcMain.handle("stop-recording", async () => {
  if (backendProcess) {
    exec(`"${backendPath}" --command stop-recording`);
    return true;
  }
  return false;
});

ipcMain.handle("get-recording-status", async () => {
  return new Promise((resolve) => {
    exec(`"${backendPath}" --command get-status`, (error, stdout) => {
      if (error) {
        console.error(`Error getting status: ${error}`);
        resolve(false);
      }
      resolve(stdout.trim() === "recording");
    });
  });
});

app.whenReady().then(() => {
  createWindow();

  app.on("activate", function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on("window-all-closed", function () {
  if (backendProcess) {
    exec(`"${backendPath}" --command stop-recording`);
  }
  if (process.platform !== "darwin") app.quit();
});

app.on("will-quit", () => {
  if (backendProcess) {
    exec(`"${backendPath}" --command exit`);
    backendProcess = null;
  }
  globalShortcut.unregisterAll();
});
