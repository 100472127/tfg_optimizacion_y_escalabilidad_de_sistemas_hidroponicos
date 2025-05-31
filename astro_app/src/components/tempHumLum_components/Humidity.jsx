import { useEffect, useState } from "react";
import useDataStore from "../../store/useDataStore";
import GaugeChart from 'react-gauge-chart';
import axios from "axios";


export default function Humidity() {
    const [humidityRead, setHumidityRead] = useState(0);
    const [HumOptMin, setHumOptMin] = useState(null);
    const [HumOptMax, setHumOptMax] = useState(null);


    useEffect(() => {
        const interval = setInterval(() => {
            const currentData = useDataStore.getState().data;
            if (currentData != null){
                setHumidityRead(parseFloat(currentData?.humidityRead));
            }
        }, 1000);

        const getInitialValues = setInterval(() => {
            const currentData = useDataStore.getState().data;
            if (currentData !== null){
                setHumOptMin(currentData?.HumOptMin);
                setHumOptMax(currentData?.HumOptMax);
                clearInterval(getInitialValues); // Limpiamos el intervalo una vez obtenidos los valores iniciales
            }
        }, 1000);

        return () => {
            clearInterval(interval);
            clearInterval(getInitialValues);
        }
    }, []);

    // Función para manejar el click de los botones de min y max
    const handleButtonClick = () => {
        let humMin, humMax;
        do {
            humMin = prompt("Ingrese el mínimo del rango óptimo de humedad (número entre 0 y 100 o 'auto' que pondrá un valor por defecto):");
            if (humMin === null) {
                return;
            }
            humMax = prompt("Ingrese el máximo del rango óptimo de humedad (número entre 0 y 100 o 'auto' que pondrá un valor por defecto):");
            if (humMax === null) {
                return;
            }
        
            const isAutoMin = humMin?.toLowerCase() === "auto";
            const isAutoMax = humMax?.toLowerCase() === "auto";
        
            const isValidNumberMin = !isNaN(humMin) && parseFloat(humMin) >= 0 && parseFloat(humMin) <= 100;
            const isValidNumberMax = !isNaN(humMax) && parseFloat(humMax) >= 0 && parseFloat(humMax) <= 100;
        
            const isValid = 
                ((isAutoMin || isValidNumberMin) &&
                 (isAutoMax || isValidNumberMax) &&
                 (isAutoMin || isAutoMax || parseFloat(humMax) >= parseFloat(humMin)));
        
            if (isValid) break;
        
            alert("VALORES NO VÁLIDOS, ASEGURATE QUE:\n- MAX > MIN\n- Valores dentro del rango (0-100)\n- Solo números o 'auto'\nPOR FAVOR INTRODUZCALOS DE NUEVO");
        
        } while (true);
        
        
        if (useDataStore.getState().actualController != null){
            const urlPost = useDataStore.getState().url + "/ajusteHum/" + useDataStore.getState().actualController;
            const range = humMin + "-" + humMax;
            axios.post(urlPost, { value: range })
            .then(response => {
                console.log(`Rango enviado ${range}:`);
                setHumOptMin(parseFloat(humMin).toFixed(1));
                setHumOptMax(parseFloat(humMax).toFixed(1));
            })
            .catch(error => {
                console.error(`Error al enviar el rango ${range}:`, error);
            });
        };
    }


    return (
        <section className="humidity-container w-full h-full flex flex-col justify-center items-center bg-theme-green pb-3 rounded-lg shadow-sm shadow-gray-800">
            <h1 className="w-full h-1/5 flex justify-center text-4xl bg-theme-dark-green rounded-lg p-2">Humedad</h1>
            <div className="w-full h-4/5 p-2 justify-center items-center flex flex-col">
                <div id="temperature-graph-container" className="flex flex-row justify-center items-center w-full h-3/4">
                    <div className="flex flex-col items-center justify-center h-full w-auto">
                        <div className="gauge-container flex flex-col tems-center justify-center  h-10 w-auto">
                            <GaugeChart
                                id="gauge-chart"
                                colors={['#FF0000', '#FFFF00','#00FF00', '#FFFF00', '#FF0000']}
                                arcsLength={[(HumOptMin/100)-0.1, 0.1, ((HumOptMax-HumOptMin)/100), 0.1, 1-(HumOptMax/100)-0.1]}
                                percent={humidityRead / 100}
                                arcWidth={0.3}
                                textColor="#000000"
                                needleColor="#C2C2C2"
                            />
                        </div>
                    </div>
                </div>
                <div id="optimal-range" className="flex flex-col border-t-1 pt-4 justify-center items-center w-60 h-1/4 gap-3">
                    <p>Optimal humidity</p>
                    <div id="min-max-container" className="flex flex-row items-center w-full justify-center gap-4">
                        <button
                            onClick={() => handleButtonClick()}
                            className="w-15 h-9 bg-theme-cream rounded-lg shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
                        >
                            {HumOptMin !== null ? HumOptMin : "min"}
                        </button>
                        <button
                            onClick={() => handleButtonClick()}
                            className="w-15 h-9 bg-theme-cream rounded-lg shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
                        >
                            {HumOptMax !== null ? HumOptMax : "min"}
                        </button>                    
                    </div>
                </div>
            </div>
        </section>
    );
}
