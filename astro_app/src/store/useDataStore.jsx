import { create } from 'zustand';

const useDataStore = create((set) => ({
    // Variable para almacenar los datos del controlador
    data: null,
    setData: (newData) => set({ data: newData }),

    // Variable para almacenar el id del controlador de la página actual
    actualController: null,
    setActualController: (newController) => set({ actualController: newController }),

    // Ip y puerto del servidor Node.js que corre en la Raspberry Pi
    url: "http://localhost:3000", 

    // Variable para compartir el estado de manualORauto entre los componetes de los timers y el control de bombas
    manualORauto: null,
    setManualORauto: (newManualORauto) => set({ manualORauto: newManualORauto }),

    // Variables para compartir el estado de los valores óptimos y mínimos de pH y TDS
    pHOptMin: null,
    setPHOptMin: (newPHOptMin) => set({ pHOptMin: newPHOptMin }),
    pHOptMax: null,
    setPHOptMax: (newPHOptMax) => set({ pHOptMax: newPHOptMax }),
    TDSOptMin: null,
    setTDSOptMin: (newTdsOptMin) => set({ TDSOptMin: newTdsOptMin }),
    TDSOptMax: null,
    setTDSOptMax: (newTdsOptMax) => set({ TDSOptMax: newTdsOptMax }),
}));

export default useDataStore;