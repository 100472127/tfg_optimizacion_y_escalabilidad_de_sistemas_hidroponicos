import { use, useEffect, useState } from "react";
import useDataStore from "../../store/useDataStore";
import axios from "axios";

export default function Temperature() {
    const [tempRead, setTempRead] = useState(null);
    const [TempOptMin, setTempOptMin] = useState(null);
    const [TempOptMax, setTempOptMax] = useState(null);
    const [tempStatus, setTempStatus] = useState("...");

    useEffect(() => {
        const interval = setInterval(() => {
            const currentData = useDataStore.getState().data;
            setTempRead(parseFloat(currentData?.tempRead).toFixed(1));
            setTempOptMin(parseFloat(currentData?.TempOptMin));
            setTempOptMax(parseFloat(currentData?.TempOptMax)); 

        }, 1000);

        return () => clearInterval(interval);
    }, []);

    useEffect(() => {
        if( TempOptMin <= tempRead && tempRead <= TempOptMax ){
            setTempStatus("GOOD");
        } else if (tempRead <= TempOptMin){
            setTempStatus("LOW");
        } else{
            setTempStatus("HIGH");
        }
    }, [tempRead, TempOptMax, TempOptMin]);


    // Función para manejar el click de los botones de min y max
    const handleButtonClick = () => {
        let tempMin, tempMax;
        do {
            tempMin = prompt("Ingrese el mínimo del rango óptimo de temperatura (número entre 0 y 100 o 'auto' que pondrá un valor por defecto):");
            tempMax = prompt("Ingrese el máximo del rango óptimo de temperatura (número entre 0 y 100 o 'auto' que pondrá un valor por defecto):");
        
            const isAutoMin = tempMin?.toLowerCase() === "auto";
            const isAutoMax = tempMax?.toLowerCase() === "auto";
        
            const isValidNumberMin = !isNaN(tempMin) && parseFloat(tempMin) >= 0 && parseFloat(tempMin) <= 100;
            const isValidNumberMax = !isNaN(tempMax) && parseFloat(tempMax) >= 0 && parseFloat(tempMax) <= 100;
        
            const isValid = 
                ((isAutoMin || isValidNumberMin) &&
                    (isAutoMax || isValidNumberMax) &&
                    (isAutoMin || isAutoMax || parseFloat(tempMax) >= parseFloat(tempMin)));
        
            if (isValid) break;
        
            alert("VALORES NO VÁLIDOS, ASEGURATE QUE:\n- MAX > MIN\n- Valores dentro del rango (0-100)\n- Solo números o 'auto'\nPOR FAVOR INTRODUZCALOS DE NUEVO");
        
        } while (true);
        
       
        if (useDataStore.getState().actualController != null){
            const urlPost = useDataStore.getState().url + "/ajusteTemp/" + useDataStore.getState().actualController;
            const range = tempMin + "-" + tempMax;
            axios.post(urlPost, { value: range })
            .then(response => {
                console.log(`Rango enviado ${range}:`);
            })
            .catch(error => {
                console.error(`Error al enviar el rango ${range}:`, error);
            });
        };
    }

    const getHeatHeight = () => {
        if (tempRead === null || isNaN(tempRead)) return "0%";
        const value = parseFloat(tempRead);
        const percentage = Math.min(100, Math.max(0, (value / 40) * 100));
        return `${percentage}%`;
      };


    const getStatusBgColor = () => {
        if(tempStatus == "GOOD"){
            return "bg-green-100";
        } else if(tempStatus == "HIGH"){
            return "bg-red-100";
        } else if(tempStatus == "LOW"){
            return "bg-blue-100";
        } else{
            return "bg-theme-cream";
        }

    }
    
    return(
        <section className="temperature-container w-full h-full flex flex-col justify-center items-center bg-theme-green pb-3 rounded-lg shadow-sm shadow-gray-800">
            <h1 className="w-full h-1/5 flex justify-center text-4xl bg-theme-dark-green rounded-lg p-2">Temperatura</h1>
            <div className="w-full h-4/5 p-2 justify-center items-center flex flex-col pl-6">
                <div id="temperature-value-graph-container" className="flex flex-row justify-center items-center w-full h-3/4">
                    <div className="w-5 h-full flex flex-col justify-between pb-4 mr-0.5">
                        <p className="flex justify-end ml-7">40º</p>
                        <p className="flex justify-end">0º</p>
                    </div>
                    <div id="temperature-graph" className=" w-auto h-full flex flex-col justify-end items-center pb-3">
                        <div className="flex flex-col items-center  h-full w-auto">
                            <div className="relative h-full w-6 top-3">
                                <div className="absolute bottom-0 w-full h-full bg-gray-300 rounded-full"></div>
                                    <div
                                        id="temperature-bar"
                                        className="absolute bottom-0 w-full bg-red-600 rounded-full"
                                        style={{ height: getHeatHeight() }}
                                        >
                                    </div>                                
                                </div>
                            <div className="w-10 h-10 bg-red-600 rounded-full"></div>
                        </div>
                    </div>
                    <div id="temperature-value-status" className=" w-auto h-full flex flex-col items-center">
                        <p className={`text-4xl rounded-lg border-1 w-30 text-center p-2 mt-2 ${getStatusBgColor()}`}>
                            {tempStatus}
                        </p>
                        <div  className=" w-auto h-full flex items-center pl-5">
                            <p className="w-auto h-15  text-center flex items-center justify-end text-5xl ">
                                {tempRead !== null ? tempRead+"ºC" : "..."}
                            </p>
                        </div>
                    </div>

                </div>
                <div id="optimal-range" className="flex flex-col border-t-1 pt-4 justify-center items-center w-60 h-1/4 gap-3">
                    <p>Optimal temperature</p>
                    <div id="min-max-container" className="flex flex-row items-center w-full justify-center gap-4">
                        <button
                            onClick={() => handleButtonClick()}
                            className="w-15 h-9 bg-theme-cream rounded-lg shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
                        >
                            {TempOptMin !== null ? TempOptMin : "min"}
                        </button> 
                        <button
                            onClick={() => handleButtonClick()}
                            className="w-15 h-9 bg-theme-cream rounded-lg shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
                        >
                            {TempOptMax !== null ? TempOptMax : "min"}
                        </button>                        
                    </div>
                </div>
            </div>
        </section>
    );
}
