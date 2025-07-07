import "./global.css";
import MainWindow from "./components/mainWindow/mainWindow";

function App() {
  return (
    <>
      <div className="flex h-screen w-screen min-h-screen">
        <MainWindow></MainWindow>
      </div>
    </>
  );
}

export default App;
