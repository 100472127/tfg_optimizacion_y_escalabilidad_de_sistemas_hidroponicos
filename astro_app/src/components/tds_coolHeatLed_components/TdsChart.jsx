import { useEffect, useState } from "react";
import { Line } from "react-chartjs-2";
import useDataStore from "../../store/useDataStore"; // Importamos Zustand

import {
	Chart as ChartJS,
	CategoryScale,
	LinearScale,
	PointElement,
	LineElement,
	Title,
	Tooltip,
	Legend,
	Filler,
} from "chart.js";

ChartJS.register(
	CategoryScale,
	LinearScale,
	PointElement,
	LineElement,
	Title,
	Tooltip,
	Legend,
	Filler
);

export default function TDSChart() {
	const [dataPoints, setDataPoints] = useState([{ x: 0, y: 0 }]); // Inicializamos con un punto (0,0) para evitar errores al renderizar el gráfico
	const [index, setIndex] = useState(1);
	const maxDatasTDS = 10;

	useEffect(() => {
		const interval = setInterval(() => {
			const phValue = useDataStore.getState().data?.waterQualityRead;
			if (phValue !== null && !isNaN(phValue)) {
				setDataPoints((prevData) => {
					const newData = [...prevData, { x: index, y: phValue }];
					if (newData.length > maxDatasTDS) newData.shift();
					return newData;
				});
				setIndex((prev) => prev + 1);
			}
		}, 1000);

		return () => clearInterval(interval);
	}, [index]);

	const chartData = {
		labels: [],
		datasets: [
			{
				label: "TDS",
				data: dataPoints,
				fill: false,
				borderColor: "rgb(75, 192, 192)",
				tension: 0.1,
			},
		],
	};

const options = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
        title: {
        display: true,
        text: "Evolución del TDS",
        },
    },
    scales: {
        x: {
        title: {
            display: false,
        },
        type: "linear",
        position: "bottom",
        ticks: {
            display: false,
        },
        },
        y: {
        title: {
            display: true,
            text: "Nivel de TDS",
        },
        min: 0,
        max: 2000,
        },
    },
};

	return (
		<div className="w-full h-48 bg-theme-white flex justify-center items-center rounded-lg shadow-md">
			<Line
				data={chartData}
				options={options}
				height={null}
				width={null}
			/>
		</div>
	);
}
