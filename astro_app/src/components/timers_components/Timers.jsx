import { use, useEffect, useState } from "react";
import useDataStore from "../../store/useDataStore";
import axios from "axios";


export default function Timers(){
    const [manualORauto, setManualORauto] = useState(0); // 0 para automático, 1 para manual
    const [sprayTimer, setSprayTimer] = useState("loading ...");
    const [pumpTimer, setPumpTimer] = useState("loading ...");

    useEffect(() => {
        // Obtenemos el estado inicial de manualORauto
        const interval = setInterval(() => {
            const currentData = useDataStore.getState().data; 
            if(currentData != null){
                setManualORauto(currentData?.manualORauto);
                useDataStore.getState().setManualORauto(currentData?.manualORauto);
                clearInterval(interval); // Limpiar el intervalo después de obtener los datos
            }
        }, 1000);

        const fetchObtainSprayAndPumpTimer = async () => {
            const id = useDataStore.getState().actualController;
            if (id !== null){
                // Obtenemos el tiempo de activación del spray
                try {
                    const response = await fetch(useDataStore.getState().url + `/tiempoUltimoSpray/${id}`);
                    if (!response.ok) {
                        throw new Error(`Error al obtener el tiempo de activación de la bomba: ${response.status}`);
                    }
                    const sprayJsonData = await response.json();
                    var actualTime = sprayJsonData.actualTime;
                    var lastSpray = sprayJsonData.lastSpray;
                    var sprayUseInterval = sprayJsonData.sprayUseInterval;
                    var activationTime = (parseFloat(lastSpray) + parseFloat(sprayUseInterval)) - parseFloat(actualTime);
                    if (activationTime < 0) {
                        setSprayTimer("0h 0' 0''");
                    } else{
                        var activationHours = parseFloat(parseInt(activationTime / 3600) % 24);
                        var activationSeconds = parseFloat(activationTime % 60);
                        var activationMinutes = parseFloat(parseInt(activationTime / 60) % 60);
                        setSprayTimer(activationHours.toString() + "h " + activationMinutes.toString() + "' " + activationSeconds.toString() + "''");
                    }
                } catch (error) {
                    console.error("Error fetching tiempoUltimoSpray:", error);
                }

                // Obtenemos el tiempo de activación de la bomba
                try {
                    const response = await fetch(useDataStore.getState().url + `/tiempoActBombaAuto/${id}`);
                    if (!response.ok) {
                        throw new Error(`Error al obtener el tiempo de activación del spray`);
                    }
    
                    const pumpJsonData = await response.json();
                    var actualTime = pumpJsonData.actualTime;
                    var counterMezcla = pumpJsonData.counterMezcla;
                    var pumpUseInterval = pumpJsonData.pumpUseInterval;
                    var activationTime = (parseFloat(counterMezcla) + parseFloat(pumpUseInterval)) - parseFloat(actualTime);
                    if (activationTime < 0) {
                        setPumpTimer("0h 0' 0''");
                    } else{
                        var activationHours = parseFloat(parseInt(activationTime / 3600) % 24);
                        var activationSeconds = parseFloat(activationTime % 60);
                        var activationMinutes = parseFloat(parseInt(activationTime / 60) % 60);
                        setPumpTimer(activationHours.toString() + "h " + activationMinutes.toString() + "' " + activationSeconds.toString() + "''");
                    }
                } catch (error) {
                    console.error("Error fetching tiempoActBombaAuto:", error);
                }
            }
        }


        const sprayPumpInterval = setInterval(fetchObtainSprayAndPumpTimer, 5000);

        return () => {
            clearInterval(interval);
            clearInterval(sprayPumpInterval);
        }
    }, []);

    const handleButtonOnOffClick = () => {
        let newManualORauto = manualORauto == 0 ? 1 : 0; // Cambia entre manual (1) y automático (0)
        const id = useDataStore.getState().actualController;
        if (id !== null){
            const urlPost = useDataStore.getState().url + `/selectControlMode/${id}`;
            axios.post(urlPost, { value: newManualORauto })
            .then(response => {
                console.log("Modo bomba enviado: ", newManualORauto);
                setManualORauto(newManualORauto); // Actualiza el estado local
                useDataStore.getState().setManualORauto(newManualORauto);
            })
            .catch(error => {
                console.error(`Error al enviar el modo bomba ${newManualORauto}:`, error);
            });
        }
    }

    const handleButtonActivarSprayClick = () => {
        const id = useDataStore.getState().actualController;
        if (id !== null){
            const urlPost = useDataStore.getState().url + `/spray/${id}`;
            axios.post(urlPost)
            .then(response => {
                console.log(`Spray activado`);
            })
            .catch(error => {
                console.error(`Error al activar el spray`, error);
            });
        }
    }

    const handleButtonSendPumpTimerClick = () => {
        if(manualORauto == 0){
            alert("El modo de control está en automático. Cambie a manual para controlar las bombas.");
        }
        else{
            let seconds;
            do {
                seconds = prompt("Introduzca valor entre activaciones de la bomba en segundos, mínimo de 3600 segundos:");
                if (seconds === null) {
                    return;
                }
                const isValid = !isNaN(seconds) && parseFloat(seconds) >= 3600;
                if (isValid) break;
                alert("VALORES NO VÁLIDOS\nASEGURATE QUE EL VALOR ES MAYOR DE 3600 Y SOLO SON NUMEROS\nPOR FAVOR INTRODUZCALOS DE NUEVO");
            } while (true);
    
            const id = useDataStore.getState().actualController;
            if (id !== null){
                const urlPost = useDataStore.getState().url + `/ajustePumpAutoUse/${id}`;
                axios.post(urlPost, { value: seconds })
                .then(response => {
                    console.log(`Pump timer establecido: ${seconds}`);
                })
                .catch(error => {
                    console.error(`Error al establecer el pump timer`, error);
                });
            }
        }
    }

    const handleButtonSendSprayTimerClick = () => {
        let seconds;
        do {
            seconds = prompt("Introduzca valor entre activaciones de la bomba en segundos, mínimo de 60 segundos:");
            if (seconds === null) {
                return;
            }
            const isValid = !isNaN(seconds) && parseFloat(seconds) >= 60;
            if (isValid) break;
            alert("VALORES NO VÁLIDOS\nASEGURATE QUE EL VALOR ES MAYOR DE 60 Y SOLO SON NUMEROS\nPOR FAVOR INTRODUZCALOS DE NUEVO");
        } while (true);

        const id = useDataStore.getState().actualController;
        if (id !== null){
            const urlPost = useDataStore.getState().url + `/ajusteSprayUse/${id}`;
            axios.post(urlPost, { value: seconds })
            .then(response => {
                console.log(`Spray timer establecido: ${seconds}`);
            })
            .catch(error => {
                console.error(`Error al establecer el spray timer`, error);
            });
        }
    }

    const getManualColor = () => {
        return manualORauto == 1 ? "bg-blue-400" : "bg-gray-400"; // Rojo
    }

    const getAutoColor = () => {
        return manualORauto == 0 ? "bg-green-400" : "bg-gray-400"; // Azul
    }  

    return (
        <section className="timers-container w-full h-full flex flex-col justify-center items-center bg-theme-green rounded-lg shadow-sm shadow-gray-800">
            <div id="pump-timer" className="h-1/2 w-full">
                <h1 className="w-full h-auto flex justify-center text-[1.6vw] pr-2 pl-2 bg-theme-dark-green rounded-lg">Temporizador Bomba</h1>
                <div className="w-full h-25 flex flex-row justify-center gap-10 items-center">
                    <div className="on-off-container">
                        <div className="flex flex-col justify-center items-center gap-2">
                            <button
                                onClick={() => handleButtonOnOffClick()}
                                className="w-25 h-8 bg-theme-cream text-center rounded-md shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
                            >
                                ON/OFF
                            </button>
                            <div className="flex flex-row justify-center items-center gap-5">
                                <div className={`w-20 h-10 text-lg ${getManualColor()} text-center rounded-md shadow-sm shadow-gray-800 flex items-center justify-center`}>MANUAL</div>
                                <div className={`w-20 h-10 text-lg ${getAutoColor()} text-center rounded-md shadow-sm shadow-gray-800 flex items-center justify-center`}>AUTO</div>
                            </div>
                        </div>
                    </div>
                    <button
                        onClick={() => handleButtonSendPumpTimerClick()}
                        className="pump-timer w-30 h-14 text-xl bg-theme-cream text-center rounded-md shadow-sm shadow-gray-800 flex items-center justify-center"
                    >
                        {pumpTimer}
                    </button>
                </div>
            </div>
            <div id="spray-timer" className="h-1/2 w-full">
                <h1 className="w-full h-auto flex justify-center text-[1.6vw] pr-2 pl-2 bg-theme-dark-green rounded-lg">Temporizador Spray</h1>
                <div className="flex flex-row w-full h-3/5 justify-center items-center gap-10">
                    <button
                        onClick={() => handleButtonActivarSprayClick()}
                        className="w-30 h-14 text-xl bg-theme-cream text-center rounded-md shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
                    >
                        ACTIVAR
                    </button>
                    <button
                        onClick={() => handleButtonSendSprayTimerClick()}
                        className="w-30 h-14 text-xl bg-theme-cream text-center rounded-md shadow-sm shadow-gray-800 flex items-center justify-center"
                    >
                        {sprayTimer}
                    </button>
                </div>
            </div>
        </section>
    );
}