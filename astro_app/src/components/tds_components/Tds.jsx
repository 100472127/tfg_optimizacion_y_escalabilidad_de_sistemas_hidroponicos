import { useEffect, useState } from "react";
import useDataStore from "../../store/useDataStore";
import TdsChart from "./TdsChart";
import axios from "axios";

export default function Tds() {
    const [waterQualityRead, setWaterQualityRead] = useState(null);
    const [TDSOptMin, setTDSOptMin] = useState(null);
    const [TDSOptMax, setTDSOptMax] = useState(null);
    const [pHOptMin, setPHOptMin] = useState(null);
    const [pHOptMax, setPHOptMax] = useState(null);
    useEffect(() => {
        const interval = setInterval(() => {
            const currentData = useDataStore.getState().data;
            if (currentData !== null){
                setWaterQualityRead(currentData?.waterQualityRead);
                setTDSOptMin(currentData?.TDSOptMin);
                setTDSOptMax(currentData?.TDSOptMax); 
                setPHOptMin(currentData?.pHOptMin);
                setPHOptMax(currentData?.pHOptMax);  
            }
        }, 1000);

        return () => clearInterval(interval);
    }, []);


    // Función para manejar el click de los botones de min y max
    const handleButtonClick = () => {
        let tdsMin, tdsMax;
        do {
            tdsMin = prompt("Ingrese el mínimo del rango óptimo de TDS (número entre 0 y 2000 o 'auto' que pondrá un valor por defecto):");
            if (tdsMin === null) {
                return;
            }
            tdsMax = prompt("Ingrese el máximo del rango óptimo de TDS (número entre 0 y 2000 o 'auto' que pondrá un valor por defecto):");
            if (tdsMax === null) {
                return;
            }
        
            const isAutoMin = tdsMin?.toLowerCase() === "auto";
            const isAutoMax = tdsMax?.toLowerCase() === "auto";
        
            const isValidNumberMin = !isNaN(tdsMin) && parseFloat(tdsMin) >= 0 && parseFloat(tdsMin) <= 2000;
            const isValidNumberMax = !isNaN(tdsMax) && parseFloat(tdsMax) >= 0 && parseFloat(tdsMax) <= 2000;
        
            const isValid = 
                ((isAutoMin || isValidNumberMin) &&
                (isAutoMax || isValidNumberMax) &&
                (isAutoMin || isAutoMax || parseFloat(tdsMax) >= parseFloat(tdsMin)));
        
            if (isValid) break;
        
            alert("VALORES NO VÁLIDOS, ASEGURATE QUE:\n- MAX > MIN\n- Valores dentro del rango (0-14)\n- Solo números o 'auto'\nPOR FAVOR INTRODUZCALOS DE NUEVO");
        
        } while (true);
        
        // El controlador necesita el rango de pH y TDS para ajustar la lógica difusa, por lo que enviaremos
        // el rango de tds nuevo y el rango de pH que estaba guardado previamente para modificar solo el tds
        if (useDataStore.getState().actualController != null){
            const urlPost = useDataStore.getState().url + "/ajustePh/" + useDataStore.getState().actualController;
            const ranges = pHOptMin + "-" + pHOptMax + ";" + tdsMin + "-" + tdsMax;
            axios.post(urlPost, { value: ranges })
            .then(response => {
                console.log(`Rango enviado ${ranges}:`);
            })
            .catch(error => {
                console.error(`Error al enviar el rango ${ranges}:`, error);
            });
        };
    }

    return(
        <section className="tds-container w-full h-full flex flex-col items-center bg-theme-green rounded-lg pb-2 shadow-sm shadow-gray-800 gap-2">
            <h1 className="w-full h-auto text-4xl flex justify-center z-20 bg-theme-dark-green rounded-lg">Calidad del Agua</h1>
            <div className="w-full h-auto flex justify-center items-center">
                <div className="tds-value-container h-full w-1/3 gap-2 flex flex-col items-center">    
                    <h1 className="w-full h-1/4 flex justify-center text-4xl">TDS</h1>
                    <div id="tds-value" className="w-40 h-15 text-center flex items-center justify-center text-5xl rounded-lg ">
                        {waterQualityRead !== null ? waterQualityRead : "..."}
                    </div>
                    <div id="optimal-range" className="flex flex-col justify-center items-center w-60 h-1/2 gap-3">
                        <div id="line" className="h-0.5 w-40 bg-black"></div>
                        <p>Optimal Range</p>
                        <div id="min-max-container" className="flex flex-row items-center w-full justify-center gap-4">
                            <button
                                onClick={() => handleButtonClick()}
                                className="w-15 h-9 bg-theme-cream rounded-lg shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
                            >
                                {TDSOptMin !== null ? TDSOptMin : "min"}
                            </button>
                            <button
                            onClick={() => handleButtonClick()}
                            className="w-15 h-9 bg-theme-cream rounded-lg shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
                            >
                                {TDSOptMax !== null ? TDSOptMax : "max"}
                            </button>
                        </div>
                    </div>
                </div>
                <div className="tds-graph-container h-55 w-2/3 flex justify-center items-center">
                    <TdsChart client:only="react"/>
                </div>
            </div>
        </section>
    );
}