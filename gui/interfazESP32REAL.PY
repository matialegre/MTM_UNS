import tkinter as tk
from tkinter import ttk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.pyplot as plt
import requests
import numpy as np
import threading
import queue
import time

class ControladorMotoresWiFi:
    def __init__(self, ip_esp32='192.168.4.1:80'):  # Ajusta la IP para apuntar al ESP32
        self.base_url = f'http://{ip_esp32}'

    def set_velocidad_disco(self, velocidad):
        url = f'{self.base_url}/set_disco?rpm={velocidad}'
        requests.get(url)

    def set_velocidad_bolita(self, velocidad):
        url = f'{self.base_url}/set_bolita?rpm={velocidad}'
        requests.get(url)

    def set_torque(self, valor_torque):
        url = f'{self.base_url}/set_torque'
        requests.post(url, data={'value': valor_torque})

    def obtener_torque(self):
        url = f'{self.base_url}/get_torque'
        response = requests.get(url)
        if response.status_code == 200:
            return float(response.text)
        return None

class App(tk.Tk):
    def __init__(self, controlador):
        super().__init__()
        self.controlador = controlador
        self.title("Sistema de Control de Torque y Velocidad")
        self.geometry("900x700")

        control_frame = ttk.LabelFrame(self, text="Control de Motores")
        control_frame.pack(pady=10, padx=10, fill="x")

        # Control de Velocidad del Disco
        ttk.Label(control_frame, text="Velocidad Disco (m/s):").grid(row=0, column=0, padx=5, pady=5)
        self.velocidad_disco = tk.DoubleVar()
        slider_disco = ttk.Scale(control_frame, from_=0, to=4, orient='horizontal', variable=self.velocidad_disco)
        slider_disco.grid(row=0, column=1, padx=5, pady=5)
        self.entry_disco = ttk.Entry(control_frame, textvariable=self.velocidad_disco, width=10)
        self.entry_disco.grid(row=0, column=2, padx=5, pady=5)
        ttk.Button(control_frame, text="Aplicar Velocidad Disco", command=self.aplicar_velocidad_disco).grid(row=0, column=3, padx=5, pady=5)

        # Control de Velocidad de la Bolita
        ttk.Label(control_frame, text="Velocidad Bolita (m/s):").grid(row=1, column=0, padx=5, pady=5)
        self.velocidad_bolita = tk.DoubleVar()
        slider_bolita = ttk.Scale(control_frame, from_=0, to=4, orient='horizontal', variable=self.velocidad_bolita)
        slider_bolita.grid(row=1, column=1, padx=5, pady=5)
        self.entry_bolita = ttk.Entry(control_frame, textvariable=self.velocidad_bolita, width=10)
        self.entry_bolita.grid(row=1, column=2, padx=5, pady=5)
        ttk.Button(control_frame, text="Aplicar Velocidad Bolita", command=self.aplicar_velocidad_bolita).grid(row=1, column=3, padx=5, pady=5)

        # Control de Tiempo Total de Prueba
        ttk.Label(control_frame, text="Tiempo Total de Prueba (segundos):").grid(row=2, column=0, padx=5, pady=5)
        self.tiempo_total = tk.IntVar(value=60)
        slider_tiempo = ttk.Scale(control_frame, from_=10, to=1800, orient='horizontal', variable=self.tiempo_total)
        slider_tiempo.grid(row=2, column=1, padx=5, pady=5)
        self.entry_tiempo = ttk.Entry(control_frame, textvariable=self.tiempo_total, width=10)
        self.entry_tiempo.grid(row=2, column=2, padx=5, pady=5)
        ttk.Button(control_frame, text="Iniciar Prueba", command=self.iniciar_prueba).grid(row=2, column=3, padx=5, pady=5)

        # Opción de variación automática de la velocidad
        self.var_auto = tk.BooleanVar(value=False)
        ttk.Checkbutton(control_frame, text="Variación Automática de Velocidad", variable=self.var_auto).grid(row=3, column=0, columnspan=4, padx=5, pady=5)

        # Control de Torque Manual
        ttk.Label(control_frame, text="Torque Manual (Nm):").grid(row=4, column=0, padx=5, pady=5)
        self.torque_manual = tk.DoubleVar(value=2.5)
        self.entry_torque = ttk.Entry(control_frame, textvariable=self.torque_manual, width=10)
        self.entry_torque.grid(row=4, column=1, padx=5, pady=5)
        ttk.Button(control_frame, text="Aplicar Torque", command=self.aplicar_torque).grid(row=4, column=3, padx=5, pady=5)

        # Gráfico de Torque en Tiempo Real
        graph_frame = ttk.LabelFrame(self, text="Torque en Tiempo Real")
        graph_frame.pack(pady=10, padx=10, fill="both", expand=True)

        self.fig, self.ax = plt.subplots(figsize=(8, 4))
        self.ax.set_title("Torque vs Tiempo")
        self.ax.set_xlabel("Tiempo (s)")
        self.ax.set_ylabel("Torque (Nm)")
        self.canvas = FigureCanvasTkAgg(self.fig, master=graph_frame)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)

        self.torque_data = np.array([])
        self.time_data = np.array([])
        self.data_queue = queue.Queue()
        self.running = False
        self.velocidad_inicial = 0

    def aplicar_velocidad_disco(self):
        velocidad = self.velocidad_disco.get()
        self.controlador.set_velocidad_disco(velocidad)

    def aplicar_velocidad_bolita(self):
        velocidad = self.velocidad_bolita.get()
        self.controlador.set_velocidad_bolita(velocidad)

    def aplicar_torque(self):
        valor_torque = self.torque_manual.get()
        self.controlador.set_torque(valor_torque)

    def iniciar_prueba(self):
        self.running = True
        self.torque_data = np.array([])
        self.time_data = np.array([])
        tiempo_total = self.tiempo_total.get()
        self.frecuencia_muestreo = max(1, tiempo_total // 60)  # Ajusta la frecuencia de muestreo
        self.velocidad_inicial = 0
        self.velocidad_final = 4
        self.incremento_velocidad = (self.velocidad_final - self.velocidad_inicial) / tiempo_total if self.var_auto.get() else 0

        # Crear y empezar el hilo para actualizar datos
        data_thread = threading.Thread(target=self.actualizar_datos)
        data_thread.daemon = True
        data_thread.start()

        # Iniciar el ciclo para actualizar la gráfica
        self.actualizar_grafico()

    def actualizar_datos(self):
        while self.running:
            torque = self.controlador.obtener_torque()
            if torque is not None:
                timestamp = self.time_data[-1] + self.frecuencia_muestreo if len(self.time_data) > 0 else 0
                self.data_queue.put((timestamp, torque))

            if self.var_auto.get():
                nueva_velocidad = self.velocidad_inicial + self.incremento_velocidad * (timestamp if len(self.time_data) > 0 else 0)
                self.controlador.set_velocidad_disco(nueva_velocidad)
                self.controlador.set_velocidad_bolita(nueva_velocidad)

            time.sleep(1 / self.frecuencia_muestreo)

    def actualizar_grafico(self):
        try:
            while not self.data_queue.empty():
                timestamp, torque = self.data_queue.get_nowait()
                self.time_data = np.append(self.time_data, timestamp)
                self.torque_data = np.append(self.torque_data, torque)

            self.ax.clear()
            self.ax.plot(self.time_data, self.torque_data, label="Torque (Nm)")
            self.ax.set_title("Torque vs Tiempo")
            self.ax.set_xlabel("Tiempo (s)")
            self.ax.set_ylabel("Torque (Nm)")
            self.ax.legend()
            self.canvas.draw()
        except queue.Empty:
            pass
        finally:
            if self.running:
                self.after(100, self.actualizar_grafico)  # Actualiza cada 100 ms

if __name__ == "__main__":
    controlador = ControladorMotoresWiFi()
    app = App(controlador)
    app.mainloop()
