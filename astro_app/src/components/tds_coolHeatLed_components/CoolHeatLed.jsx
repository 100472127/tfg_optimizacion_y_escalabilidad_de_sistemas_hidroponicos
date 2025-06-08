import { useEffect, useState, useRef } from "react";
import useDataStore from "../../store/useDataStore";
import axios from "axios";

export default function CoolHeatLed() {
	const [ledMode, setLedMode] = useState(0); // 0 automático, 1 manual
	const [fanMode, setfanMode] = useState(0);
	const [heaterMode, setHeaterMode] = useState(0);
	const [ledPower, setLedPower] = useState(null);
	const [fanPower, setFanPower] = useState(null);
	const [heaterPower, setHeaterPower] = useState(null);

    // Refs para mantener los valores actualizados
	const ledModeRef = useRef(ledMode);
	const fanModeRef = useRef(fanMode);
	const heaterModeRef = useRef(heaterMode)

    // Sincronizamos las refs con el estado actual en cada render
	useEffect(() => {
		ledModeRef.current = ledMode;
		fanModeRef.current = fanMode;
		heaterModeRef.current = heaterMode;
	}, [ledMode, fanMode, heaterMode]);

	useEffect(() => {
		const getInitialValues = setInterval(() => {
			const currentData = useDataStore.getState().data;
			if (currentData !== null) {
				setLedMode(currentData?.manualORautoLed);
				setfanMode(currentData?.manualORautoFan);
				setHeaterMode(currentData?.manualORautoHeater);
				setLedPower(currentData?.powerLed);
				setFanPower(currentData?.powerFan);
				setHeaterPower(currentData?.powerHeater);
                clearInterval(getInitialValues); // Limpiar el intervalo después de obtener los datos
			}
		}, 1000);

        // Si está en automático (0), la lógica difusa actualiza los valores de potencia y tenemos que actualizarlos en la interfaz
        const updatePowerValues = setInterval(() => {
            const currentData = useDataStore.getState().data;
            if (currentData !== null) {
				if (ledModeRef.current === 0) {
					setLedPower(currentData?.powerLed);
				}
				if (fanModeRef.current === 0) {
					setFanPower(currentData?.powerFan);
				}
				if (heaterModeRef.current === 0) {
					setHeaterPower(currentData?.powerHeater);
				}
            }
        }, 5000);

        // Limpiar el intervalo cuando el componente se desmonte
        return () => {
            clearInterval(getInitialValues);
            clearInterval(updatePowerValues);
        };
	}, []);

	const handleBgColor = (mode) => {
        if (mode == 1) {
            return "bg-green-500 text-white";
        } else {
            return "bg-gray-400 text-white";
        }
	};


	const handleMode = (actuator) => {
        const urlPost = useDataStore.getState().url + "/mode" + actuator + "/" + useDataStore.getState().actualController;
        let mode = 0;
        if (actuator == "Led") {
            mode = ledMode == 0 ? 1 : 0;
            setLedPower(0);
        } else if (actuator == "Fan") {
            mode = fanMode == 0 ? 1 : 0;
            setFanPower(0);
        } else if (actuator == "Heater") {
            mode = heaterMode == 0 ? 1 : 0;
            setHeaterPower(0);
        }
        axios.post(urlPost, { value: mode.toString()})
        .then(response => {
            console.log("Modo del " + actuator + " actualizado: ", mode);
            if (actuator == "Led") {
                setLedMode(mode);
            } else if (actuator == "Fan") {
                setfanMode(mode);
            } else if (actuator == "Heater") {
                setHeaterMode(mode);
            }
        })
        .catch(error => {
            console.error("Error al actualizar el modo del " + actuator + ":", error);
        });
    };

	const handlePower = (actuator) => {
        if (actuator == "Led" && ledMode == 1 || actuator == "Fan" && fanMode == 1 || actuator == "Heater" && heaterMode == 1) {
            let power;
            do {
                power = prompt("Ingrese la potencia del " + actuator + " (0-10):");
                if (power === null) {
                    return;
                }
                if (isNaN(power) || power < 0 || power > 10) {
                    alert("Potencia no válida, debe ser un número entre 0 y 10.");
                } else {
                    break;
                }
            } while (true);

            if (useDataStore.getState().actualController != null) {
                const urlPost = useDataStore.getState().url + "/power" + actuator + "/" + useDataStore.getState().actualController;
                console.log(urlPost);
                axios.post(urlPost, { value: power })
                .then(response => {
                    console.log("Potencia del " + actuator + " actualizada:", response.data);
                    if (actuator == "Led") {
                        setLedPower(power);
                    } else if (actuator == "Fan") {
                        setFanPower(power);
                    } else if (actuator == "Heater") {
                        setHeaterPower(power);
                    }
                })
                .catch(error => {
                    console.error("Error al actualizar la potencia del " + actuator + ":", error);
                });
            }
        } else {
            alert("El " + actuator + " está en modo automático, no se puede ajustar la potencia manualmente.");
        }
	};

	return (
		<section className="tds-container w-full h-full flex flex-col items-center bg-theme-green rounded-lg pb-2 shadow-sm shadow-gray-800 gap-2">
			<h1 className="w-full h-auto text-4xl flex justify-center z-20 bg-theme-dark-green rounded-lg">
				Actuadores
			</h1>
			<div className="w-full h-full grid grid-cols-3 grid-rows-4 gap-2 p-4">
				{/* Fila de encabezados */}
				<div className="flex items-center justify-center font-semibold">
					Nada
				</div>
				<div className="flex items-center justify-center font-semibold">
					Control <br></br> manual
				</div>
				<div className="flex items-center justify-center font-semibold">
					Potencia <br></br> (1–10)
				</div>

				{/* Fila LED */}
				<div className="flex items-center justify-center">LED</div>
				<div className="flex items-center justify-center">
					<button
						onClick={() => handleMode("Led")}
						className={`px-4 py-2 rounded shadow hover:bg-gray-200 ${handleBgColor(ledMode)}`}
					>
                        ON/OFF
					</button>
				</div>
				<div className="flex items-center justify-center">
					<button
						onClick={() => handlePower("Led")}
						className="w-17 h-9 bg-theme-cream rounded-md shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
					>
						{ledPower !== null ? ledPower : "input"}
					</button>
				</div>

				{/* Fila Ventilador */}
				<div className="flex items-center justify-center">
					Ventilador
				</div>
				<div className="flex items-center justify-center">
					<button
						onClick={() => handleMode("Fan")}
						className={`px-4 py-2 rounded shadow hover:bg-gray-200 ${handleBgColor(fanMode)}`}
					>
                        ON/OFF
					</button>
				</div>
				<div className="flex items-center justify-center">
					<button
						onClick={() => handlePower("Fan")}
						className="w-17 h-9 bg-theme-cream rounded-md shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
					>
						{fanPower !== null ? fanPower : "input"}
					</button>
				</div>

				{/* Fila Heater */}
				<div className="flex items-center justify-center">Calentador</div>
				<div className="flex items-center justify-center">
					<button
						onClick={() => handleMode("Heater")}
						className={`px-4 py-2 rounded shadow hover:bg-gray-200 ${handleBgColor(heaterMode)}`}
					>
                        ON/OFF
					</button>
				</div>
				<div className="flex items-center justify-center">
					<button
						onClick={() => handlePower("Heater")}
						className="w-17 h-9 bg-theme-cream rounded-md shadow-sm shadow-gray-800 flex items-center justify-center hover:bg-theme-dark-green transition"
					>
						{heaterPower !== null ? heaterPower : "input"}
					</button>
				</div>
			</div>
		</section>
	);
}
