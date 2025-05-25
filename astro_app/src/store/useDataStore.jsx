import { create } from 'zustand';

const useDataStore = create((set) => ({
  data: null,
  setData: (newData) => set({ data: newData }),

  actualController: null,
  setActualController: (newController) => set({ actualController: newController }),

  url: "http://localhost:3000"
}));

export default useDataStore;