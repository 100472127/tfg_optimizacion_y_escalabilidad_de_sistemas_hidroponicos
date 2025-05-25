import { useEffect, useState } from "react";
import PropTypes from "prop-types"; // Importamos PropTypes para validación

const MonitoringItem = ({ id }) => {
    const [ph, setPh] = useState(null);
    const [waterQuality, setWaterQuality] = useState(null);
    const [temp, setTemp] = useState(null);
    const [humidity, setHumidity] = useState(null);
    const [light, setLight] = useState(null);
    const [lowWater, setLowWater] = useState(null);

    const url = `http://localhost:3000/data/${id}`;

    useEffect(() => {
        const obtainData = async () => {
            try {
                console.log(url);
                const response = await fetch(url);
                const jsonData = await response.json();

                if (response.ok) {
                    setPh(jsonData.phRead);
                    setWaterQuality(jsonData.waterQualityRead);
                    setTemp(jsonData.tempRead);
                    setHumidity(jsonData.humidityRead);
                    setLight(jsonData.lightSensorRead);
                    setLowWater(jsonData.lowWaterRead);
                } else {
                    console.error("Error al obtener datos:", jsonData.error);
                }
            } catch (error) {
                console.error("Error al obtener datos:", error);
            }
        };

        obtainData();
        const interval = setInterval(obtainData, 5000); // Actualizar cada 5 segundos

        return () => clearInterval(interval); // Limpiar intervalo al desmontar el componente
    }, [id]);

    return (
        <div className="w-full max-h-[450px] h-auto p-4 border rounded-lg bg-theme-green shadow-md flex flex-col gap-2  text-gray-900">
            <h3 className="font-bold text-3xl text-center h-1/4">Controlador {id}</h3>

            <div className="grid grid-cols-3 gap-2 text-center text-xl justify-center h-3/4">
                <div className="bg-white rounded flex flex-col gap-2 shadow">
                    <p>PH</p>
                    <p>{ph !== null ? ph : "Cargando..."}</p>
                </div>
                <div className="bg-white p-2 flex flex-col gap-2 rounded shadow">
                    <p>Temperatura</p>
                    <p>{temp !== null ? `${temp} °C` : "Cargando..."}</p>
                </div>
                <div className="bg-white p-2 flex flex-col gap-2 rounded shadow">
                    <p>Humedad</p>
                    <p>{humidity !== null ? `${humidity} %` : "Cargando..."}</p>
                </div>
                <div className="bg-white p-2 flex flex-col gap-2 rounded shadow">
                    <p>Calidad del Agua</p>
                    <p>{waterQuality !== null ? waterQuality : "Cargando..."}</p>
                </div>
                <div className="bg-white p-2 flex flex-col gap-2 rounded shadow">
                    <p>Luz</p>
                    <p>{light !== null ? light : "Cargando..."}</p>
                </div>
                <div className="bg-white p-2 flex flex-col gap-2 rounded shadow">
                    <p>Nivel Bajo de Agua</p>
                    <p>{lowWater !== null ? (lowWater ? "Sí" : "No") : "Cargando..."}</p>
                </div>
            </div>
        </div>
    );
};


MonitoringItem.propTypes = {
    id: PropTypes.string.isRequired,
};

export default MonitoringItem;
