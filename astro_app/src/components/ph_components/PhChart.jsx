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

export default function PhChart() {
	const [dataPoints, setDataPoints] = useState([{ x: 0, y: 0 }]);
	const [index, setIndex] = useState(1);
	const maxDatasPH = 10;

	useEffect(() => {
		const interval = setInterval(() => {
			const phValue = useDataStore.getState().data?.phRead;
			if (phValue !== null && !isNaN(phValue)) {
				setDataPoints((prevData) => {
					const newData = [...prevData, { x: index, y: phValue }];
					if (newData.length > maxDatasPH) newData.shift();
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
				label: "PH",
				data: dataPoints,
				fill: false,
				borderColor: "rgb(75, 192, 192)",
				tension: 0.1,
			},
		],
	};

	const options = {
		plugins: {
			title: {
				display: true,
				text: "Evolución del pH",
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
					text: "Nivel de pH",
				},
				min: 0,
				max: 14,
			},
		},
	};

	return (
		<div className="w-full h-full bg-theme-white flex justify-center items-center rounded-lg shadow-md">
			<Line
				data={chartData}
				options={options}
				height={null}
				width={null}
			/>
		</div>
	);
}
