import { useEffect, useState } from "react";
import useDataStore from "../../store/useDataStore";
import axios from "axios";

export default function PumpControl() {
    const [statusBombaAcida, setStatusBombaAcida] = useState(false);
    const [statusBombaAlcalina, setStatusBombaAlcalina] = useState(false);
    const [statusBombaMezcla, setStatusBombaMezcla] = useState(false);
    const [statusBombaNutrientes, setStatusBombaNutrientes] = useState(false);
    const [statusBombaMezclaResiduos, setStatusBombaMezclaResiduos] = useState(false);
    const [statusBombaResiduos, setStatusBombaResiduos] = useState(false);

    useEffect(() => {
        // Intervalo para obtener el estaado de las bombas por primera vez
        const statusPumpInterval = setInterval(() => {
            const currentData = useDataStore.getState().data; 
            if(currentData !== null) {
                // Si las bombas están activas establecemos el estado a true, si están inactivas a false
                setStatusBombaAcida(currentData?.statusBombaAcida == 1.0);
                setStatusBombaAlcalina(currentData?.statusBombaAlcalina == 1.0);
                setStatusBombaMezcla(currentData?.statusBombaMezcla == 1.0);
                setStatusBombaNutrientes(currentData?.statusBombaNutrientes == 1.0);
                setStatusBombaMezclaResiduos(currentData?.statusBombaMezclaResiduos == 1.0);
                setStatusBombaResiduos(currentData?.statusBombaResiduos == 1.0);
                clearInterval(statusPumpInterval); // Limpiar el intervalo después de obtener los datos
            }
        }, 1000);

        // Limpiar el intervalo cuando el componente se desmonte
        return () => {
            clearInterval(statusPumpInterval);
        };
    }, []);

    // Función para manejar el cambio de estado de los checkboxes y enviar la información al controlador
    const handleToggle = (setter, prevStatus, pump) => {
        if (useDataStore.getState().actualController != null) { 
            if (useDataStore.getState().manualORauto == 1) {
                const urlPost = useDataStore.getState().url + "/" + pump + "/" + useDataStore.getState().actualController;
                const newStatus = prevStatus ? 0.0 : 1.0; // Convertir el estado booleano a 1.0 o 0.0

                axios.post(urlPost, { value: newStatus })
                .then(response => {
                    setter((prev) => !prev);
                    console.log(`${pump} actualizada a ${newStatus}:`);
                    if(pump === "bombaAcida") {
                        setStatusBombaAcida(newStatus === 1.0);
                    } else if(pump === "bombaAlcalina") {
                        setStatusBombaAlcalina(newStatus === 1.0);
                    } else if(pump === "bombaMezcla") {
                        setStatusBombaMezcla(newStatus === 1.0);
                    } else if(pump === "bombaNutrientes") {
                        setStatusBombaNutrientes(newStatus === 1.0);
                    } else if(pump === "bombaMezclaResiduos") {
                        setStatusBombaMezclaResiduos(newStatus === 1.0);
                    } else if(pump === "bombaResiduos") {
                        setStatusBombaResiduos(newStatus === 1.0);
                    }
                })
                .catch(error => {
                    console.error(`Error al actualizar ${pump}:`, error);
                });
            } else{
                alert("El modo de control está en automático. Cambie a manual para controlar las bombas.");
            }
        };
    }

    return (
        <section className="pump-control-container w-full h-full flex flex-col items-center bg-theme-green rounded-lg shadow-sm shadow-gray-800">
            <h1 className="w-full h-1/5 flex flex-row justify-center text-4xl bg-theme-dark-green rounded-lg p-3">
                Control de Bombas
            </h1>
            <div id="pump-buttons" className="grid grid-cols-2 grid-rows-3 gap-4 w-full h-4/5 p-4 bg-theme-green">
                <div id="boton-acida" className="flex flex-col justify-center items-center">
                    <h6>Bomba Ácida</h6>
                    <label className="relative items-center cursor-pointer">
                        <input
                            type="checkbox"
                            className="sr-only peer"
                            checked={statusBombaAcida}
                            onChange={() => handleToggle(setStatusBombaAcida, statusBombaAcida, "bombaAcida")} // Llamamos a la función que actualiza el estado
                        />
                        <div className="w-30 h-10 bg-gray-300 peer-focus:ring-4 peer-focus:ring-blue-300 rounded-full peer peer-checked:after:translate-x-20 peer-checked:after:border-white after:content-[''] after:absolute after:top-1 after:left-0.5 after:bg-white after:border-gray-300 after:border after:rounded-full after:h-8 after:w-8 after:transition-all peer-checked:bg-blue-600"></div>
                    </label>
                </div>
                <div id="boton-alcalina" className="flex flex-col justify-center items-center">
                    <h6>Bomba Alcalina</h6>
                    <label className="relative items-center cursor-pointer">
                        <input
                            type="checkbox"
                            className="sr-only peer"
                            checked={statusBombaAlcalina}
                            onChange={() => handleToggle(setStatusBombaAlcalina, statusBombaAlcalina, "bombaAlcalina")} // Llamamos a la función que actualiza el estado
                        />
                        <div className="w-30 h-10 bg-gray-300 peer-focus:ring-4 peer-focus:ring-blue-300 rounded-full peer peer-checked:after:translate-x-20 peer-checked:after:border-white after:content-[''] after:absolute after:top-1 after:left-0.5 after:bg-white after:border-gray-300 after:border after:rounded-full after:h-8 after:w-8 after:transition-all peer-checked:bg-blue-600"></div>
                    </label>
                </div>
                <div id="boton-mezcla" className="flex flex-col justify-center items-center">
                    <h6>Bomba Mezcla</h6>
                    <label className="relative items-center cursor-pointer">
                        <input
                            type="checkbox"
                            className="sr-only peer"
                            checked={statusBombaMezcla}
                            onChange={() => handleToggle(setStatusBombaMezcla, statusBombaMezcla, "bombaMezcla")} // Llamamos a la función que actualiza el estado
                        />
                        <div className="w-30 h-10 bg-gray-300 peer-focus:ring-4 peer-focus:ring-blue-300 rounded-full peer peer-checked:after:translate-x-20 peer-checked:after:border-white after:content-[''] after:absolute after:top-1 after:left-0.5 after:bg-white after:border-gray-300 after:border after:rounded-full after:h-8 after:w-8 after:transition-all peer-checked:bg-blue-600"></div>
                    </label>
                </div>
                <div id="boton-nutrientes" className="flex flex-col justify-center items-center">
                    <h6>Bomba Nutrientes</h6>
                    <label className="relative items-center cursor-pointer">
                        <input
                            type="checkbox"
                            className="sr-only peer"
                            checked={statusBombaNutrientes}
                            onChange={() => handleToggle(setStatusBombaNutrientes, statusBombaNutrientes, "bombaNutrientes")} // Llamamos a la función que actualiza el estado
                        />
                        <div className="w-30 h-10 bg-gray-300 peer-focus:ring-4 peer-focus:ring-blue-300 rounded-full peer peer-checked:after:translate-x-20 peer-checked:after:border-white after:content-[''] after:absolute after:top-1 after:left-0.5 after:bg-white after:border-gray-300 after:border after:rounded-full after:h-8 after:w-8 after:transition-all peer-checked:bg-blue-600"></div>
                    </label>
                </div>
                <div id="boton-mez-res" className="flex flex-col justify-center items-center">
                    <h6>Bomba Mez-Res</h6>
                    <label className="relative items-center cursor-pointer">
                        <input
                            type="checkbox"
                            className="sr-only peer"
                            checked={statusBombaMezclaResiduos}
                            onChange={() => handleToggle(setStatusBombaMezclaResiduos, statusBombaMezclaResiduos, "bombaMezclaResiduos")} // Llamamos a la función que actualiza el estado
                        />
                        <div className="w-30 h-10 bg-gray-300 peer-focus:ring-4 peer-focus:ring-blue-300 rounded-full peer peer-checked:after:translate-x-20 peer-checked:after:border-white after:content-[''] after:absolute after:top-1 after:left-0.5 after:bg-white after:border-gray-300 after:border after:rounded-full after:h-8 after:w-8 after:transition-all peer-checked:bg-blue-600"></div>
                    </label>
                </div>
                <div id="boton-residuos" className="flex flex-col justify-center items-center">
                    <h6>Bomba Residuos</h6>
                    <label className="relative items-center cursor-pointer">
                        <input
                            type="checkbox"
                            className="sr-only peer"
                            checked={statusBombaResiduos}
                            onChange={() => handleToggle(setStatusBombaResiduos, statusBombaResiduos, "bombaResiduos")} // Llamamos a la función que actualiza el estado
                        />
                        <div className="w-30 h-10 bg-gray-300 peer-focus:ring-4 peer-focus:ring-blue-300 rounded-full peer peer-checked:after:translate-x-20 peer-checked:after:border-white after:content-[''] after:absolute after:top-1 after:left-0.5 after:bg-white after:border-gray-300 after:border after:rounded-full after:h-8 after:w-8 after:transition-all peer-checked:bg-blue-600"></div>
                    </label>
                </div>
            </div>
        </section>
    );
}
