// components/DataUpdater.jsx
import { useEffect } from "react";
import useDataStore from "../store/useDataStore";

export default function DataUpdater({ id }) {
    const setData = useDataStore((state) => state.setData);
    const setActualController = useDataStore((state) => state.setActualController);

    useEffect(() => {
        if (!id) return;

        // Guardamos el id del controlador actual en el store
        setActualController(id);

        // Función para obtener datos del servidor para el controlador actual
        const fetchData = async () => {
            try {
                const urlGet = useDataStore.getState().url + "/data/" + id;
                const response = await fetch(urlGet);
                if (!response.ok) {
                    throw new Error(`Error al obtener datos: ${response.status}`);
                }

                const jsonData = await response.json();
                setData(jsonData);

            } catch (error) {
                console.error("Error fetching data:", error);
            }
        };

        fetchData(); // Llama a la función inmediatamente para obtener datos al cargar el componente

        // Cada 5 segundos traemos nuevos datos del servidor para el controlador actual
        const interval = setInterval(fetchData, 5000);

        return () => clearInterval(interval);
    }, [id, setData]);

    return null;
}
