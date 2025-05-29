import { useEffect, useState } from "react";
import useDataStore from "../../store/useDataStore";
import PhChart from "./PhChart";
import axios from "axios";

export default function Ph() {
    const [phRead, setPhRead] = useState(null);
    const [pHOptMin, setPHOptMin] = useState(null);
    const [pHOptMax, setPHOptMax] = useState(null);
    const [TDSOptMin, setTdsOptMin] = useState(null);
    const [TDSOptMax, setTdsOptMax] = useState(null);
    useEffect(() => {
        const interval = setInterval(() => {
            const currentData = useDataStore.getState().data;
            if (currentData !== null){
                setPhRead(currentData?.phRead);
                setPHOptMin(currentData?.pHOptMin);
                setPHOptMax(currentData?.pHOptMax); 
                setTdsOptMin(currentData?.TDSOptMin);
                setTdsOptMax(currentData?.TDSOptMax);
            }
        }, 1000);

        return () => clearInterval(interval);
    }, []);

    // Función para manejar el click de los botones de min y max
    const handleButtonClick = () => {
        let pHMin, pHMax;
        do {
            pHMin = prompt("Ingrese el mínimo del rango óptimo de pH (número entre 0 y 14 o 'auto' que pondrá un valor por defecto):");
            if (pHMin === null) {
                return;
            }
            pHMax = prompt("Ingrese el máximo del rango óptimo de pH (número entre 0 y 14 o 'auto' que pondrá un valor por defecto):");
            if (pHMax === null) {
                return;
            }
        
            const isAutoMin = pHMin?.toLowerCase() === "auto";
            const isAutoMax = pHMax?.toLowerCase() === "auto";
        
            const isValidNumberMin = !isNaN(pHMin) && parseFloat(pHMin) >= 0 && parseFloat(pHMin) <= 14;
            const isValidNumberMax = !isNaN(pHMax) && parseFloat(pHMax) >= 0 && parseFloat(pHMax) <= 14;
        
            const isValid = 
                ((isAutoMin || isValidNumberMin) &&
                 (isAutoMax || isValidNumberMax) &&
                 (isAutoMin || isAutoMax || parseFloat(pHMax) >= parseFloat(pHMin)));
        
            if (isValid) break;
        
            alert("VALORES NO VÁLIDOS, ASEGURATE QUE:\n- MAX > MIN\n- Valores dentro del rango (0-14)\n- Solo números o 'auto'\nPOR FAVOR INTRODUZCALOS DE NUEVO");
        
        } while (true);
        
        // El controlador necesita el rango de pH y TDS para ajustar la lógica difusa, por lo que enviaremos
        // el rango de pH nuevo y el rango de TDS que estaba guardado previamente para modificar solo el pH
        if (useDataStore.getState().actualController != null){
            const urlPost = useDataStore.getState().url + "/ajustePh/" + useDataStore.getState().actualController;
            const ranges = pHMin + "-" + pHMax + ";" + TDSOptMin + "-" + TDSOptMax;
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
        <section className="ph-container w-full h-full flex flex-row justify-center items-center bg-theme-green rounded-lg p-2 shadow-sm shadow-gray-800 gap-2">
            <div className="ph-value-container h-full w-1/3 gap-2 flex flex-col items-center">    
                <h1 className="w-full h-1/4 flex justify-center text-4xl bg-theme-dark-green rounded-lg items-center">PH Actual</h1>
                <div id="ph-value" className="w-40 h-15 text-center flex items-center justify-center text-5xl rounded-lg ">
                    {phRead !== null ? phRead : "..."}
                </div>
                <div id="optimal-range" className="flex flex-col justify-center items-center w-60 h-1/2 gap-3">
                    <div id="line" className="h-0.5 w-40 bg-black"></div>
                    <p>Optimal Range</p>
                    <div id="min-max-container" className="flex flex-row items-center w-full justify-center gap-4">
                        <button
                            onClick={() => handleButtonClick()}
                            className="w-15 h-9 bg-theme-cream rounded-lg shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
                        >
                            {pHOptMin !== null ? pHOptMin : "min"}
                        </button>
                        <button
                            onClick={() => handleButtonClick()}
                            className="w-15 h-9 bg-theme-cream rounded-lg shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
                        >
                            {pHOptMax !== null ? pHOptMax : "max"}
                        </button>
                    </div>
                </div>
            </div>
            <div className="ph-graph-container h-60 w-2/3 flex justify-center items-center p-2">
                <PhChart phValue={phRead}/>
            </div>
        </section>
    );
}