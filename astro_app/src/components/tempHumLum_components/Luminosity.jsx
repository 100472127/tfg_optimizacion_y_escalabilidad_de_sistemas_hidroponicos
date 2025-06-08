import { useEffect, useState } from "react";
import useDataStore from "../../store/useDataStore";
import ObtainLumHrs from "./ObtainLumHrs";
import axios from "axios";

export default function Luminosity() {
    const [lightSensorRead, setLightSensorRead] = useState(null);
    const [lightResistorRead, setLightResistorRead] = useState(null);
    const [LumOptMin, setLumOptMin] = useState(null);
    const [LumOptMax, setLumOptMax] = useState(null);
    const [LumOptHrsMin, setLumOptHrsMin] = useState(null);
    const [LumOptHrsMax, setLumOptHrsMax] = useState(null);

    useEffect(() => {
        const interval = setInterval(() => {
            const currentData = useDataStore.getState().data;
            if (currentData !== null){
                setLightSensorRead(parseFloat(currentData?.lightSensorRead).toFixed(1));
                setLightResistorRead(currentData?.lightResistorRead);
            }
        }, 1000);

        const getInitialValues = setInterval(() => {
            const currentData = useDataStore.getState().data;
            if (currentData !== null){
                setLumOptMin(parseFloat(currentData?.LumOptMin).toFixed(1)); 
                setLumOptMax(parseFloat(currentData?.LumOptMax).toFixed(1));
                setLumOptHrsMin(currentData?.LumOptHrsMin);
                setLumOptHrsMax(currentData?.LumOptHrsMax); 
                clearInterval(getInitialValues); // Limpiamos el intervalo una vez obtenidos los valores iniciales
            }
        }, 1000);

        return () => {
            clearInterval(interval);
            clearInterval(getInitialValues);
        }
    }, []);

    
    // Función para manejar el click de los botones de min y max
    const handleButtonClick = (type) => {
        let minValue, maxValue;
        do {
            if(type === "luminosity") {
                minValue = prompt("Ingrese el mínimo del rango óptimo de luminosidad (número entre 0 y 50000 o 'auto' que pondrá un valor por defecto):");
                if (minValue === null) {
                    return;
                }
                maxValue = prompt("Ingrese el máximo del rango óptimo de temperatura (número entre 0 y 50000 o 'auto' que pondrá un valor por defecto):");
                if (maxValue === null) {
                    return;
                }
            } else{
                minValue = prompt("Ingrese el mínimo del rango óptimo de horas de luz diarias (número entre 0 y 24 o 'auto' que pondrá un valor por defecto):");
                if (minValue === null) {
                    return;
                }
                maxValue = prompt("Ingrese el máximo del rango óptimo de horas de luz diarias (número entre 0 y 24 o 'auto' que pondrá un valor por defecto):");
                if (maxValue === null) {
                    return;
                }
            }
            const maxValueNum = type === "luminosity" ? 50000 : 24;
        
            const isAutoMin = minValue?.toLowerCase() === "auto";
            const isAutoMax = maxValue?.toLowerCase() === "auto";
        
            const isValidNumberMin = !isNaN(minValue) && parseFloat(minValue) >= 0 && parseFloat(minValue) <= maxValueNum;
            const isValidNumberMax = !isNaN(maxValue) && parseFloat(maxValue) >= 0 && parseFloat(maxValue) <= maxValueNum;
        
            const isValid = 
                ((isAutoMin || isValidNumberMin) &&
                    (isAutoMax || isValidNumberMax) &&
                    (isAutoMin || isAutoMax || parseFloat(maxValue) >= parseFloat(minValue)));
        
            if (isValid) break;
        
            alert("VALORES NO VÁLIDOS, ASEGURATE QUE:\n- MAX > MIN\n- Valores dentro del rango\n- Solo números o 'auto'\nPOR FAVOR INTRODUZCALOS DE NUEVO");
        
        } while (true);
        
       
        if (useDataStore.getState().actualController != null){
            var urlPost = null;
            if (type === "luminosity") {
                urlPost = useDataStore.getState().url + "/ajusteLum/" + useDataStore.getState().actualController;
            } else{
                urlPost = useDataStore.getState().url + "/ajusteLumHours/" + useDataStore.getState().actualController;
            }
            const range = minValue + "-" + maxValue;
            axios.post(urlPost, { value: range })
            .then(response => {
                console.log(`Rango enviado ${range}:`);
                if (type === "luminosity") {
                    setLumOptMin(parseFloat(minValue));
                    setLumOptMax(parseFloat(maxValue));
                } else {
                    setLumOptHrsMin(minValue);
                    setLumOptHrsMax(maxValue);
                }
            })
            .catch(error => {
                console.error(`Error al enviar el rango ${range}:`, error);
            });
        };
    }

    const getLightMarkerPosition = () => {
        if (parseFloat(lightSensorRead) >= parseFloat(LumOptMax)) return '100%';
        if (parseFloat(lightSensorRead) <= parseFloat(LumOptMin)) return '0%';
        const percentage = ((lightSensorRead - LumOptMin) / (LumOptMax - LumOptMin)) * 100;
        return `${percentage}%`;
      };

    return (
        <section className="luminosity-container w-full h-full flex flex-col justify-center items-center pb-3 bg-theme-green rounded-lg shadow-sm shadow-gray-800">
            <h1 className="w-full h-1/5 flex justify-center text-4xl bg-theme-dark-green rounded-lg p-2">Luminosidad</h1>
            <div className="w-full h-4/5 pb-2 justify-center items-center flex flex-col">
                <div className="w-full h-3/4 flex flex-col justify-center items-center">  
                    <div id="luminosity-container" className="w-full h-1/2 flex flex-col justify-center items-center gap-2">
                        <p className="h-1/2 w-auto text-5xl">
                            {lightSensorRead}
                        </p>
                        <div className="h-6 w-3/4 relative bg-gradient-to-r from-red-500 via-yellow-500 to-green-500 rounded-lg">
                            <div
                                className="absolute top-0 z-100 h-full rounded-lg w-1 bg-blue-700"
                                style={{
                                    left: getLightMarkerPosition(),
                                    transform: 'translateX(-50%)',
                                }}
                            ></div>
                        </div>
                    </div>
                    <div className="w-full h-1/2 flex flex-col justify-center items-center">
                        <div id="luminosity-resistor-container" className="w-full h-1/2 flex flex-row pl-5 pr-5 items-center justify-between">
                            <p className="w-auto h-auto text-[1vw] text-center">Resistencia lumínica</p>
                            <p className="w-auto h-auto text-center flex items-center justify-center text-[1vw] p-1 pl-2 pr-2">
                                {lightResistorRead}
                            </p>
                        </div>
                        <div id="luminosity-resistor-container" className="w-full h-1/2 flex flex-row pl-5 pr-5 items-start justify-between">
                            <p className="w-auto h-auto text-[1vw] text-center">Hrs luz obtenidas</p>
                            <p className="w-auto h-auto text-center flex items-center justify-center text-[1vw] p-1 pl-2 pr-2">
                                <ObtainLumHrs/>
                            </p>
                        </div> 
                    </div>
                </div>
                <div className="h-1/4 flex flex-row justify-center gap-4 pt-4 items-center border-t-1">
                    <div id="optimal-range-left" className="flex flex-col justify-center items-center w-auto h-full gap-3 ">
                        <p>Optimal luminosity</p>
                        <div id="min-max-container" className="flex flex-row items-center w-full justify-center gap-4">
                            <button
                                onClick={() => handleButtonClick("luminosity")}
                                className="w-15 h-9 bg-theme-cream rounded-lg shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
                            >
                                {LumOptMin !== null ? LumOptMin : "min"}
                            </button>
                            <button
                                onClick={() => handleButtonClick("luminosity")}
                                className="w-15 h-9 bg-theme-cream rounded-lg shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
                            >
                                {LumOptMax !== null ? LumOptMax : "max"}
                            </button>
                        </div>
                    </div>
                    <div id="optimal-range-right" className="flex flex-col justify-center items-center w-auto h-full gap-3">
                        <p>Optimal light hrs</p>
                        <div id="min-max-container" className="flex flex-row items-center w-full justify-center gap-4">
                            <button
                                onClick={() => handleButtonClick("lightHrs")}
                                className="w-15 h-9 bg-theme-cream rounded-lg shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
                            >
                                {LumOptHrsMin !== null ? LumOptHrsMin : "min"}
                            </button>
                            <button
                                onClick={() => handleButtonClick("lightHrs")}
                                className="w-15 h-9 bg-theme-cream rounded-lg shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
                            >
                                {LumOptHrsMax !== null ? LumOptHrsMax : "max"}
                            </button>
                        </div>
                    </div>
                </div>
            </div>
        </section>
    );
}