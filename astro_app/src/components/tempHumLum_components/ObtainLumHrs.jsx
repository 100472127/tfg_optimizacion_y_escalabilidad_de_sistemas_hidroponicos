import { useEffect, useState } from "react";
import useDataStore from "../../store/useDataStore";

export default function ObtainLumHrs() {
    const [hrs, setHrs] = useState("...");

    useEffect(() => {

        const fetchObtainLumHrs = async () => {
            const id = useDataStore.getState().actualController;
            if (id !== null){
                try {
                    const response = await fetch(`http://localhost:3000/horasLuz/${id}`);
                    if (!response.ok) {
                        throw new Error(`Error al obtener datos de LumHrs: ${response.status}`);
                    }
    
                    const jsonData = await response.json();
                    const lightHours = jsonData?.horasLuz;
                    var timeData = parseFloat(lightHours);
                    
                    var hours = parseFloat(parseInt(timeData / 3600) % 24);
                    var seconds = parseFloat(timeData % 60);
                    var minutes = parseFloat(parseInt(timeData / 60) % 60);
    
                    setHrs(hours.toString() + "h " + minutes.toString() + "' " + seconds.toString() + "''");
    
                } catch (error) {
                    console.error("Error fetching LumHrs:", error);
                }
            }
        }

        const interval = setInterval(fetchObtainLumHrs, 5000);

        return () => clearInterval(interval);
    }, []);

    return hrs;
}
