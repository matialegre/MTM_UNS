import tkinter as tk
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from controladores.controlador_motor import ControladorMotor
from tkinter import filedialog, messagebox
import json

class VentanaPrincipal:
    def __init__(self, root):
        self.root = root
        self.root.title("Control de Torque y Velocidad")
        self.controlador_motor = ControladorMotor()
        
        # Configuración de la ventana
        self.frame = tk.Frame(root)
        self.frame.pack(pady=10, padx=10)

        # Menú de archivo para guardar y cargar configuraciones
        self.menu_bar = tk.Menu(root)
        file_menu = tk.Menu(self.menu_bar, tearoff=0)
        file_menu.add_command(label="Cargar Configuración", command=self.cargar_configuracion)
        file_menu.add_command(label="Guardar Configuración", command=self.guardar_configuracion)
        self.menu_bar.add_cascade(label="Archivo", menu=file_menu)
        root.config(menu=self.menu_bar)
        
        # Entrada para la velocidad del motor 1
        self.label_vel1 = tk.Label(self.frame, text="Velocidad Motor 1 (RPM):")
        self.label_vel1.grid(row=0, column=0, padx=5, pady=5)
        
        self.entrada_vel1 = tk.Entry(self.frame)
        self.entrada_vel1.grid(row=0, column=1, padx=5, pady=5)
        
        # Botón para iniciar el motor 1
        self.boton_iniciar = tk.Button(self.frame, text="Iniciar Motor", command=self.iniciar_motor)
        self.boton_iniciar.grid(row=1, column=0, columnspan=2, pady=10)
        
        # Configuración del gráfico
        self.figura = Figure(figsize=(5, 5), dpi=100)
        self.grafico = self.figura.add_subplot(111)
        self.grafico.set_title("Torque vs. Tiempo")
        self.grafico.set_xlabel("Tiempo (s)")
        self.grafico.set_ylabel("Torque (Nm)")
        
        self.canvas = FigureCanvasTkAgg(self.figura, master=self.frame)
        self.canvas.get_tk_widget().grid(row=2, column=0, columnspan=2)
    
    def iniciar_motor(self):
        try:
            rpm1 = int(self.entrada_vel1.get())
            self.controlador_motor.enviar_comando_motor1(rpm1)
            self.actualizar_grafico()
        except ValueError:
            messagebox.showerror("Error", "Ingrese un valor numérico para RPM")
    
    def actualizar_grafico(self):
        torque_data = self.controlador_motor.obtener_datos_torque()
        self.grafico.clear()
        self.grafico.plot(torque_data['tiempo'], torque_data['torque'])
        self.canvas.draw_idle()  # Usa draw_idle para optimización.

    def guardar_configuracion(self):
        configuracion = {
            'velocidad_motor1': self.entrada_vel1.get(),
        }
        archivo = filedialog.asksaveasfilename(defaultextension=".json", filetypes=[("Archivos JSON", "*.json")])
        if archivo:
            with open(archivo, 'w') as f:
                json.dump(configuracion, f)
            messagebox.showinfo("Guardar Configuración", "Configuración guardada exitosamente.")
    
    def cargar_configuracion(self):
        archivo = filedialog.askopenfilename(defaultextension=".json", filetypes=[("Archivos JSON", "*.json")])
        if archivo:
            with open(archivo, 'r') as f:
                configuracion = json.load(f)
            self.entrada_vel1.delete(0, tk.END)
            self.entrada_vel1.insert(0, configuracion['velocidad_motor1'])
            messagebox.showinfo("Cargar Configuración", "Configuración cargada exitosamente.")
