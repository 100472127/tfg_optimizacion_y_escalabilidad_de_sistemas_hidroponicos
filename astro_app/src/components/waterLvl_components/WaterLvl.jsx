import { useEffect, useState } from "react";
import useDataStore from "../../store/useDataStore";

export default function WaterLvl() {
    const [waterLvlMinPlanRead, setWaterLvlMinPlanRead] = useState(null);
    const [waterLvlMaxPlanRead, setWaterLvlMaxPlanRead] = useState(null);
    const [waterLvlMinMezRead, setWaterLvlMinMezRead] = useState(null);
    const [waterLvlMaxMezRead, setWaterLvlMaxMezRead] = useState(null);
    const [waterLvlMaxResRead, setWaterLvlMaxResRead] = useState(null);

    useEffect(() => {
        const interval = setInterval(() => {
            const currentData = useDataStore.getState().data; 
            setWaterLvlMinPlanRead(currentData?.waterLvlMinPlanRead);
            setWaterLvlMaxPlanRead(currentData?.waterLvlMaxPlanRead);
            setWaterLvlMinMezRead(currentData?.waterLvlMinMezRead);
            setWaterLvlMaxMezRead(currentData?.waterLvlMaxMezRead);
            setWaterLvlMaxResRead(currentData?.waterLvlMaxResRead);
        }, 1000); // 1 minuto de intervalo para la lectura los sensores de nivel de agua
      
        // Limpiar el intervalo cuando el componente se desmonte
        return () => clearInterval(interval);
    }, []);


    // Lógica para asignar el color de cada círculo
    const getMainCircleColor = () => {
        if (waterLvlMinPlanRead == 0) return "bg-red-200"; // Rojo
        if (waterLvlMaxPlanRead == 1) return "bg-blue-200"; // Azul
        return "bg-theme-white"; // Blanco
    };

    const getMixCircleColor = () => {
        if (waterLvlMinMezRead == 0) return "bg-red-200"; // Rojo
        if (waterLvlMaxMezRead == 1) return "bg-blue-200"; // Azul
        return "bg-theme-white"; // Blanco
    };

    const getResCircleColor = () => {
        if (waterLvlMaxResRead == 1) return "bg-blue-200"; // Azul
        return "bg-theme-white"; // Blanco
    };

    return(
        <section className="water-lvl-container w-full h-full flex flex-col items-center rounded-lg pb-2 shadow-sm shadow-gray-800 bg-theme-green">
            <h1 className="w-full h-auto flex justify-center text-4xl bg-theme-dark-green rounded-lg p-2">Nivel del Agua</h1>
            <div id="circles-lvl" className="flex flex-row justify-center items-center w-full h-3/6 space-x-7">
                <div id="mainCircle" className="flex flex-col justify-center items-center">
                    <p>Main</p>
                    <div id="main-water-lvl" className={`w-25 h-25 border rounded-full ${getMainCircleColor()}`}></div>
                </div>
                <div id="mix-circle" className="flex flex-col justify-center items-center">
                    <p>Mezcla</p>
                    <div id="mixWaterLvl" className={`w-25 h-25 border rounded-full ${getMixCircleColor()}`}></div>
                </div>
                <div id="mix-res-circle" className="flex flex-col justify-center items-center">
                    <p>Residuos</p>
                    <div id="resWaterLvl" className={`w-25 h-25 border rounded-full ${getResCircleColor()}`}></div>
                </div>
            </div>
            <div id="info-lvl" className="h-2/6 w-19/20 p-3 bg-theme-cream rounded-lg border border-gray-800">
                <p>
                    Rojo: Nivel de agua insuficiente <br/> 
                    Blanco: Nivel de agua correcto <br/>
                    Azul: Nivel de agua desbordante
                </p>
            </div>
        </section>
    );
}