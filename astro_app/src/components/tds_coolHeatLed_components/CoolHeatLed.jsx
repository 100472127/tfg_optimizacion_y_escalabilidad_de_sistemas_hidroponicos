import { useEffect, useState } from "react";
import useDataStore from "../../store/useDataStore";
import axios from "axios";

export default function CoolHeatLed() {
	const [ledMode, setLedMode] = useState(1); // 0 automático, 1 manual
	const [fanMode, setfanMode] = useState(0);
	const [heaterMode, setHeaterMode] = useState(0);
	const [ledPower, setLedPower] = useState(null);
	const [fanPower, setFanPower] = useState(null);
	const [heaterPower, setHeaterPower] = useState(null);

	useEffect(() => {
		const interval = setInterval(() => {
			const currentData = useDataStore.getState().data;
			if (currentData !== null) {
				setLedMode(currentData?.manualORautoLED);
				setfanMode(currentData?.manualORautoFan);
				setHeaterMode(currentData?.manualORautoHeater);
				setLedPower(currentData?.ledPower);
				setFanPower(currentData?.fanPower);
				setHeaterPower(currentData?.heaterPower);
			}
		}, 2000);
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
        if (actuator === "Led") {
            setLedMode(ledMode === 0 ? 1 : 0);
            mode = ledMode;
        } else if (actuator === "Fan") {
            setfanMode(fanMode === 0 ? 1 : 0);
            mode = fanMode;
        } else if (actuator === "Heater") {
            setHeaterMode(heaterMode === 0 ? 1 : 0);
            mode = heaterMode;
        }
        axios.post(urlPost, { value: mode.toString()})
        .then(response => {
            console.log("Modo del " + actuator + " actualizado: ", mode);
        })
        .catch(error => {
            console.error("Error al actualizar el modo del " + actuator + ":", error);
        });
    };

	const handlePower = (actuator) => {
        if (actuator === "Led" && ledMode == 1 || actuator === "Fan" && fanMode == 1 || actuator === "Heater" && heaterMode == 1) {
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
                axios.post(urlPost, { value: power })
                .then(response => {
                    console.log("Potencia del " + actuator + " actualizada:", response.data);
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
				<div className="flex items-center justify-center">Heater</div>
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
