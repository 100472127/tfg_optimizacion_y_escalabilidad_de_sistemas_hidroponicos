import express from 'express';
import logger from 'morgan';
import fs from 'fs';
import axios from 'axios';
import cors from 'cors';
import pool from './db/db.js';


const port = process.env.PORT ?? 3000;

const app = express();

// Permitimos peticiones desde la URL donde se alojará la página web hecha con astro
const allowedOrigins = [
    'http://localhost:4321', // URL de desarrollo
    'http://192.168.73.200:4321', // URL de producción
];
app.use(cors({
    origin: function (origin, callback) {
        if (!origin || allowedOrigins.includes(origin)) {
            callback(null, true);
        } else {
            callback(new Error('Not allowed by CORS'));
        }
    }   
}));

// Variables para el manejo de ficheros
const controllersIpFilePath = "./server/controllersIP.json"
const controllerDataPath = "./server/sensors_data"

app.use(logger("dev"));
app.use(express.urlencoded({ extended: true }));
app.use(express.json());


// ---------------------------------
// | MICROCONTROLLER IP ASSIGNMENT |
// ---------------------------------

// El servidor necesita saber las IP de los distintos microcontroladores para saber a cuál enviar información,
// para ello, al inicializare el microcontrolador (setup) enviará su identificador e ip asignada a este servidor
// y quedará registrada en un archivo json que el servidor utilizará para localizarlos

app.post('/ipAssignment', (req, res) => {
    const newIpAssignment = req.body; // {"id": "IP"}

    // Leer el archivo controllersIP.json
    fs.readFile(controllersIpFilePath, 'utf8', (err, data) => {
        let jsonData = {};

        if (!err) {
            try {
                jsonData = JSON.parse(data); // Intentar parsear el JSON existente
            } catch (parseErr) {
                console.error("Error al parsear controllersIP.json:", parseErr);
                return res.status(500).send("Internal server error");
            }
        }

        // Agregar la nueva asignación (sobrescribiendo si ya existe)
        const id = Object.keys(newIpAssignment)[0];
        jsonData[id] = newIpAssignment[id];

        // Escribir el archivo con el contenido actualizado
        fs.writeFile(controllersIpFilePath, JSON.stringify(jsonData, null, 2), (writeErr) => {
            if (writeErr) {
                console.error("Error al escribir en controllersIP.json:", writeErr);
                return res.status(500).send("Internal server error");
            }
            res.status(200).send("IP guardada correctamente");
        });
    });
});


// --------
// | DATA |
// --------

// Almacenaremos los datos en un fichero para cada microcontrolador (Sería mejor una DB pero de momento se queda así)

// La página web pide la información de los sensores
app.get('/data/:id', (req, res) => {
    const id = req.params.id;               // Obtener el valor de la variable 'id' en la URL
    const fileName = `${controllerDataPath}/controllerData${id}.json`;     // Nombre del archivo correspondiente al 'id'

    // Leer el archivo correspondiente
    fs.readFile(fileName, 'utf8', (err, data) => {
        if (err) {
            // Si el archivo no existe o hay un error al leerlo
            console.error('Error leyendo el archivo:', err);
            res.status(500).send('Internal server Error');
        } else {
            // Si el archivo se lee correctamente, devolver los datos
            res.json(JSON.parse(data));  // Parsear el JSON y enviarlo en la respuesta
        }
    });
});

// El microcontrolador envía información de los sensores y los almacena en ficheros JSON y en la base de datos
app.post('/data/:id', async (req, res) => {
    const dataToSave = req.body;        // Obtener datos del cuerpo de la petición
    const JsonDataToSave = JSON.stringify(dataToSave, null, 2); // Convertir a JSON
    const id = req.params.id;           // Obtener el id incluido en la URL
    const fileName = `${controllerDataPath}/controllerData${id}.json`;  // Obtener el nombre del fichero en el que se guardan los datos

    // Guardamos los datos en el fichero json que servirá los datos a la página web
    fs.writeFile(fileName, JsonDataToSave, (err) => {
        if (err) {
            console.error("Error al abrir el archivo:", err);
            return res.status(500).send('Internal server Error'); // Importante agregar return aquí
        }
    });

    // Guardamos los datos en la base de datos para futuras aplicaciones
    try{
        const {
            phRead,
            waterQualityRead,
            tempRead,
            humidityRead,
            lightResistorRead,
            lightSensorRead
        } = dataToSave;

        const sql = `
        INSERT INTO sensor_readings (
            controller_id, ph_read, water_quality_read, temp_read, humidity_read,
            light_resistor_read, light_sensor_read
        ) VALUES (?, ?, ?, ?, ?, ?, ?)
        `;

    
        await pool.execute(sql, [
            id,
            phRead,
            waterQualityRead,
            tempRead,
            humidityRead,
            lightResistorRead,
            lightSensorRead
        ]);

        console.log("Datos insertados en la base de datos.");
        res.status(200).send("OK");

    } catch (dbErr) {
        console.error("Error al insertar en la base de datos:", dbErr);
        res.status(500).send("Error al guardar en la base de datos.");
    }
});


// ----------------------
// | FUNCIONES DE APOYO |
// ----------------------

// Obtiene la URL a la que se enviará la petición POST en la función sendPetition("POST", )
function getControllerURL(id, petition) {
    try {
        const data = fs.readFileSync(controllersIpFilePath, 'utf8');
        const parsedData = JSON.parse(data);
        // Obtener la IP asignada al ID
        const controllerIP = parsedData[id];
        if (!controllerIP) {
            return null; // Retorna null si no existe el ID en el JSON
        }
        return `http://${controllerIP}${petition}`;
    } catch (err) {
        console.error("Error al leer el archivo controllersIP.json:", err);
        return null;
    }
}

// Envía una petición POST (petition) con información (dataToSend) a un controlador (controllerID)
async function sendPetition(type, controllerID, dataToSend, petition) {
    const controllerURL = getControllerURL(controllerID, petition);
    console.log(controllerURL);
    if (!controllerURL) {
        return "ERROR"; // Si no se encuentra una URL, retornamos error
    }
    try {
        if (type === "POST") {
            const res = await axios.post(controllerURL, dataToSend);
            return res.data;
        } else {
            const res = await axios.get(controllerURL, dataToSend);
            return res.data;
        }        
    } catch (error) {
        console.error(`Error al enviar ${type} a`, controllerURL, error.message);
        return "ERROR";
    }
}


// -------------------------------------
// | INPUTS DEL USUARIO EN LA INTERFAZ |
// -------------------------------------

// Checkboxes de activación manual de las bombas

app.post('/bombaAcida/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("POST", controllerID, dataToSend, "/bombaAcida");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});
app.post('/bombaAlcalina/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("POST", controllerID, dataToSend, "/bombaAlcalina");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});
app.post('/bombaMezcla/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("POST", controllerID, dataToSend, "/bombaMezcla");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});
app.post('/bombaNutrientes/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("POST", controllerID, dataToSend, "/bombaNutrientes");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});
app.post('/bombaMezclaResiduos/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("POST", controllerID, dataToSend, "/bombaMezclaResiduos");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});
app.post('/bombaResiduos/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("POST", controllerID, dataToSend, "/bombaResiduos");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});

// Botón para activar el spray manualmente
app.post('/spray/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("POST", controllerID, dataToSend, "/spray");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});

// Botón para cambiar el control de las bombas de automático a manual y viceversa
app.post('/selectControlMode/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("POST", controllerID, dataToSend, "/selectControlMode");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});

// Temporizadores de las bombas y el spray cuando funcionan de forma automática
app.post('/ajustePumpAutoUse/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("POST", controllerID, dataToSend, "/ajustePumpAutoUse");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});
app.post('/ajusteSprayUse/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("POST", controllerID, dataToSend, "/ajusteSprayUse");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});

// Input barra de luminosidad o la casilla "valoir mínimo óptimo de luz"
app.post('/ajusteLum/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("POST", controllerID, dataToSend, "/ajusteLum");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});

// Input casilla "horas de luz necesarias"
app.post('/ajusteLumHours/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("POST", controllerID, dataToSend, "/ajusteLumHours");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});

// Input casilla "Rango óptimo" en PH
app.post('/ajustePh/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("POST", controllerID, dataToSend, "/ajustePh");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});

// Input casilla "Rango óptimo" en temperatura
app.post('/ajusteTemp/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("POST", controllerID, dataToSend, "/ajusteTemp");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});

// Input casilla "Rango óptimo" en humedad
app.post('/ajusteHum/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("POST", controllerID, dataToSend, "/ajusteHum");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});


// ----------------------------------------------------------------------------------------------------
// | PETICIONES ESPECÍFICAS DEL SERVIDOR GENERAL AL MICROCONTROLADOR PARA ACTUALIZACIÓN EN TIEMPO REAL|
// ----------------------------------------------------------------------------------------------------

// Las horas de luz obtenidas, el temporizador de la bomba y el temporizador del spray
// deben actualizar sus valores en tiempo real en la página web. Para ello estas peticiones
// se realizan cada segundo para obtener los valores actualizados.

// Horas de luz obtenidas
app.get('/horasLuz/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    //const result = await sendPetition("GET", controllerID, dataToSend, "/horasLuz");  // Esperar el resultado de sendPost
    //res.status(result === "ERROR" ? 500 : 200).send(result);
    res.status(200).send("{\"horasLuz\": \"59955\"}"); // Enviar respuesta al microcontrolador
});

// Temporizador del spray
app.get('/tiempoUltimoSpray/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("GET", controllerID, dataToSend, "/tiempoUltimoSpray");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});

// Temporizador de la bomba
app.get('/tiempoActBombaAuto/:id', async (req, res) => {
    const controllerID = req.params.id;  // Obtener el ID del controlador desde la URL
    const dataToSend = req.body;         // Obtener los datos del cuerpo de la petición
    const result = await sendPetition("GET", controllerID, dataToSend, "/tiempoActBombaAuto");  // Esperar el resultado de sendPost
    res.status(result === "ERROR" ? 500 : 200).send(result);
});


// Inicializamos el servidor principal
app.listen(port, '0.0.0.0', () => {
    console.log(`Server running on port ${port}`)
})