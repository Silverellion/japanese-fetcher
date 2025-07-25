const { app, BrowserWindow, globalShortcut, ipcMain } = require("electron");
const path = require("path");
const { spawn } = require("child_process");
const fs = require("fs");
const isDev = process.env.NODE_ENV !== "production";

process.env.ELECTRON_DISABLE_SECURITY_WARNINGS = "true";
app.commandLine.appendSwitch("disable-features", "Autofill");

const backendPath = path.join(__dirname, "../../backend/cpp/x64/Debug/cpp.exe");
let backendProcess = null;
let backendReady = false;
let pendingStatusResolvers = [];
let mainWindow = null;
let watcher = null;
let processedFiles = new Set();

function watchTranscripts() {
  const TRANSCRIPT_DIR = "C:\\japanese-fetcher\\Cache\\Transcripts";
  if (watcher) return;

  watcher = fs.watch(TRANSCRIPT_DIR, (eventType, filename) => {
    if (filename && filename.endsWith(".txt")) {
      const filePath = path.join(TRANSCRIPT_DIR, filename);
      if (!processedFiles.has(filePath)) {
        try {
          const content = fs.readFileSync(filePath, "utf8");
          mainWindow?.webContents.send("new-transcript", content.trim());
          processedFiles.add(filePath);
        } catch (err) {
          if (err.code !== "ENOENT") processedFiles.delete(filePath);
        }
      }
    }
  });
}

function startBackend() {
  if (backendProcess) {
    if (!backendReady) {
      return new Promise((resolve) => pendingStatusResolvers.push(resolve));
    }
    return Promise.resolve(true);
  }

  return new Promise((resolve) => {
    backendProcess = spawn(backendPath, [], {
      stdio: ["pipe", "pipe", "pipe"],
      windowsHide: true,
    });

    backendProcess.stdout.setEncoding("utf8");
    backendProcess.stderr.setEncoding("utf8");

    backendProcess.stdout.on("data", (data) => {
      if (data.includes("Backend running")) {
        backendReady = true;
        mainWindow?.webContents.send("backend-ready");
        pendingStatusResolvers.forEach((r) => r(true));
        pendingStatusResolvers = [];
        resolve(true);
        watchTranscripts();
      }
    });

    backendProcess.on("exit", () => {
      backendProcess = null;
      backendReady = false;
      if (watcher) {
        watcher.close();
        watcher = null;
      }
    });
  });
}

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1280,
    height: 720,
    minWidth: 960,
    minHeight: 540,
    show: false,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: true,
      preload: path.join(__dirname, "preload.cjs"),
    },
  });

  mainWindow.once("ready-to-show", () => {
    mainWindow.show();
    startBackend();
  });

  if (isDev) {
    mainWindow.loadURL("http://localhost:5173");
    mainWindow.webContents.openDevTools();
  } else {
    mainWindow.loadFile(path.join(__dirname, "../dist/index.html"));
  }
}

ipcMain.handle("start-recording", async () => {
  await startBackend();
  if (backendProcess && backendReady) {
    backendProcess.stdin.write("start-recording\n");
    return true;
  }
  return false;
});

ipcMain.handle("stop-recording", async () => {
  if (backendProcess && backendReady) {
    backendProcess.stdin.write("stop-recording\n");
  }
  return false;
});

ipcMain.handle("get-recording-status", async () => {
  if (!backendProcess || !backendReady) return false;
  return new Promise((resolve) => {
    pendingStatusResolvers.push(resolve);
    backendProcess.stdin.write("get-status\n");
  });
});

app.whenReady().then(createWindow);

app.on("window-all-closed", () => {
  if (watcher) {
    watcher.close();
    watcher = null;
  }
  if (backendProcess) {
    backendProcess.stdin.write("exit\n");
    backendProcess.kill();
  }
  if (process.platform !== "darwin") app.quit();
});

app.on("activate", () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow();
});
