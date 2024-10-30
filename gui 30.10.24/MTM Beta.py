import serial
import serial.tools.list_ports
import threading
import time
import pandas as pd
import tkinter as tk
from tkinter import messagebox, filedialog, simpledialog, W, E, N, S, END
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.animation import FuncAnimation

# Importar ttkbootstrap
import ttkbootstrap as ttkb
from ttkbootstrap.constants import *

# Variables globales para almacenar los datos
datos_lista = []

# Límite de muestras
limite_muestras = 1000  # Puedes ajustar este valor según tus necesidades

# Evento para detener el hilo
stop_event = threading.Event()

class App:
    def __init__(self, root):
        self.root = root
        self.root.title("Control y Monitoreo del Sistema")
        self.root.geometry("1200x800")
        self.root.resizable(True, True)

        # Estilos y temas
        self.style = ttkb.Style('flatly')  # Puedes probar otros temas: 'superhero', 'cosmo', 'darkly'
        
        # Crear un PanedWindow horizontal
        paned_window = ttkb.PanedWindow(self.root, orient=HORIZONTAL)
        paned_window.pack(fill=BOTH, expand=True)

        # Panel de Control
        control_frame = ttkb.Frame(paned_window, padding=10)
        paned_window.add(control_frame, weight=1)

        # Crear un PanedWindow vertical dentro del control_frame
        control_paned = ttkb.PanedWindow(control_frame, orient=VERTICAL)
        control_paned.pack(fill=BOTH, expand=True)

        # Sección de selección de puerto y conexión
        conexion_frame = ttkb.Frame(control_paned)
        control_paned.add(conexion_frame, weight=0)

        ttkb.Label(conexion_frame, text="Puerto COM:", font=('Helvetica', 12, 'bold')).pack(pady=5)
        self.combobox_ports = ttkb.Combobox(conexion_frame, state='readonly')
        self.combobox_ports.pack(pady=5)
        self.actualizar_puertos()
        
        ttkb.Button(conexion_frame, text="Actualizar Puertos", command=self.actualizar_puertos).pack(pady=5)

        # Estado de conexión
        self.estado_conexion = ttkb.Label(conexion_frame, text="Desconectado", font=('Helvetica', 12))
        self.estado_conexion.pack(pady=5)

        # Botón de Conectar/Desconectar
        self.boton_conectar = ttkb.Button(conexion_frame, text="Conectar", command=self.conectar_desconectar)
        self.boton_conectar.pack(pady=5)

        # Separador
        ttkb.Separator(conexion_frame, orient=HORIZONTAL).pack(fill=X, pady=10)

        # Campos para parámetros
        parametros_frame = ttkb.LabelFrame(control_paned, text="Parámetros", padding=10)
        control_paned.add(parametros_frame, weight=0)

        parametros = [
            ("R (cm):", "5.4"),
            ("r (cm):", "4.0"),
            ("RPM inicial Disco:", "0.0"),
            ("RPM final Disco:", "300.0"),
            ("Coeficiente de fricción (μ):", "0.2"),
            ("Tiempo total (s):", "60.0"),
            ("Peso aplicado (kg):", "0.0"),
            ("Límite de muestras:", "1000")  # Nuevo parámetro
        ]

        for idx, (label_text, default_value) in enumerate(parametros):
            self.crear_campo(parametros_frame, label_text, default_value, row=idx)

        # Añadir opciones para la dirección de los motores
        direcciones_frame = ttkb.LabelFrame(control_paned, text="Dirección de Motores", padding=10)
        control_paned.add(direcciones_frame, weight=0)

        # Variables para almacenar la dirección seleccionada
        self.dir_disco_var = tk.IntVar(value=1)  # 1 para HIGH, 0 para LOW
        self.dir_bola_var = tk.IntVar(value=1)

        ttkb.Label(direcciones_frame, text="Dirección Disco:").pack(anchor=W)
        ttkb.Radiobutton(direcciones_frame, text='Horario', variable=self.dir_disco_var, value=1).pack(anchor=W)
        ttkb.Radiobutton(direcciones_frame, text='Antihorario', variable=self.dir_disco_var, value=0).pack(anchor=W)

        ttkb.Label(direcciones_frame, text="Dirección Bola:").pack(anchor=W)
        ttkb.Radiobutton(direcciones_frame, text='Horario', variable=self.dir_bola_var, value=1).pack(anchor=W)
        ttkb.Radiobutton(direcciones_frame, text='Antihorario', variable=self.dir_bola_var, value=0).pack(anchor=W)

        # Botones de control
        botones_frame = ttkb.Frame(control_paned)
        control_paned.add(botones_frame, weight=0)

        ttkb.Button(botones_frame, text="Enviar Parámetros", command=self.enviar_parametros, bootstyle=PRIMARY).pack(fill=X, pady=5)
        ttkb.Button(botones_frame, text="Iniciar Ensayo", command=self.enviar_start, bootstyle=SUCCESS).pack(fill=X, pady=5)
        ttkb.Button(botones_frame, text="Detener Ensayo", command=self.enviar_stop, bootstyle=DANGER).pack(fill=X, pady=5)
        ttkb.Button(botones_frame, text="Reiniciar Datos", command=self.reiniciar_datos, bootstyle=WARNING).pack(fill=X, pady=5)
        ttkb.Button(botones_frame, text="Guardar Datos", command=self.guardar_datos, bootstyle=INFO).pack(fill=X, pady=5)
        ttkb.Button(botones_frame, text="Info Ecuaciones", command=self.mostrar_ecuaciones, bootstyle=INFO).pack(fill=X, pady=5)

        # Área de Mensajes y Cálculos
        mensajes_frame = ttkb.LabelFrame(control_paned, text="Mensajes y Cálculos", padding=10)
        control_paned.add(mensajes_frame, weight=1)

        self.text_area = tk.Text(mensajes_frame)
        self.text_area.pack(side=LEFT, fill=BOTH, expand=True)

        scrollbar_text = ttkb.Scrollbar(mensajes_frame, orient=VERTICAL, command=self.text_area.yview)
        self.text_area.configure(yscrollcommand=scrollbar_text.set)
        scrollbar_text.pack(side=RIGHT, fill=Y)

        # Frame para gráficos y datos
        data_frame = ttkb.Frame(paned_window)
        paned_window.add(data_frame, weight=3)

        # Crear un PanedWindow horizontal dentro del data_frame
        data_paned = ttkb.PanedWindow(data_frame, orient=HORIZONTAL)
        data_paned.pack(fill=BOTH, expand=True)

        # Área de Gráficos
        graficos_frame = ttkb.LabelFrame(data_paned, text="Visualización de Datos", padding=10)
        data_paned.add(graficos_frame, weight=3)

        self.fig = plt.Figure(figsize=(7, 6))
        self.canvas = FigureCanvasTkAgg(self.fig, master=graficos_frame)
        self.canvas.get_tk_widget().pack(side=TOP, fill=BOTH, expand=True)

        self.ani = FuncAnimation(self.fig, self.actualizar_grafica, interval=500, cache_frame_data=False)  # Intervalo de 500 ms

        # Panel de Valores Actuales
        valores_frame = ttkb.LabelFrame(data_paned, text="Valores Actuales", padding=10)
        data_paned.add(valores_frame, weight=1)

        # Diccionario para almacenar las etiquetas
        self.valores_actuales = {}

        variables = [
            'Velocidad Disco (m/s)',
            'Velocidad Bola (m/s)',
            'Torque (kg)',
            'Duty Disco (%)',
            'Duty Bola (%)'
        ]

        for var in variables:
            frame = ttkb.Frame(valores_frame)
            frame.pack(anchor='w', pady=2)
            label_var = ttkb.Label(frame, text=f"{var}:", font=('Helvetica', 10))
            label_var.pack(side='left')
            value_label = ttkb.Label(frame, text="0.00", font=('Helvetica', 10, 'bold'))
            value_label.pack(side='left')
            # Guardamos la etiqueta en el diccionario
            self.valores_actuales[var] = value_label

        # Registro de Datos
        datos_frame = ttkb.LabelFrame(data_frame, text="Datos Adquiridos", padding=10)
        datos_frame.pack(side=BOTTOM, fill=BOTH, expand=True)

        columns = ('Tiempo', 'Duty Disco', 'Velocidad Disco', 'Duty Bola', 'Velocidad Bola', 'Torque', 'T_pulse_D', 'T_pulse_B')
        self.tree = ttkb.Treeview(datos_frame, columns=columns, show='headings')
        for col in columns:
            self.tree.heading(col, text=col)
            self.tree.column(col, width=100)
        self.tree.pack(side=LEFT, fill=BOTH, expand=True)

        scrollbar = ttkb.Scrollbar(datos_frame, orient=VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side=RIGHT, fill=Y)

        # Variables de control
        self.ser = None
        self.hilo_serial = None

    def crear_campo(self, frame, label_text, default_value, row):
        label = ttkb.Label(frame, text=label_text)
        label.grid(row=row, column=0, sticky=W, pady=5)
        # Determinar los rangos y pasos según el parámetro
        if "RPM" in label_text:
            from_, to, increment = 0, 10000, 10
        elif "Tiempo" in label_text:
            from_, to, increment = 0, 1000, 1
        elif "μ" in label_text:
            from_, to, increment = 0, 1, 0.01
        elif "Peso" in label_text:
            from_, to, increment = 0, 1000, 0.1
        elif "Límite" in label_text:
            from_, to, increment = 100, 10000, 100
        else:
            from_, to, increment = 0, 1000, 0.1
        spinbox = ttkb.Spinbox(frame, from_=from_, to=to, increment=increment)
        spinbox.set(default_value)
        spinbox.grid(row=row, column=1, pady=5)
        attribute_name = label_text.split('(')[0].strip().replace(' ', '_').replace(':', '').replace('μ', 'mu').replace('í', 'i').replace('é', 'e')
        setattr(self, attribute_name + '_entry', spinbox)

    def actualizar_puertos(self):
        ports = serial.tools.list_ports.comports()
        port_list = [port.device for port in ports]
        self.combobox_ports['values'] = port_list
        if port_list:
            self.combobox_ports.current(0)

    def conectar_desconectar(self):
        if self.ser and self.ser.is_open:
            # Desconectar
            self.ser.close()
            self.estado_conexion.config(text="Desconectado")
            self.boton_conectar.config(text="Conectar")
            self.text_area.insert(END, "Desconectado del puerto serial.\n")
            self.text_area.see(END)
        else:
            try:
                port = self.combobox_ports.get()
                if not port:
                    messagebox.showerror("Error", "No se ha seleccionado un puerto COM.")
                    return
                self.ser = serial.Serial(port, 115200, timeout=1)
                time.sleep(2)  # Esperar a que se establezca la conexión
                self.estado_conexion.config(text="Conectado")
                self.boton_conectar.config(text="Desconectar")
                self.text_area.insert(END, f"Conectado al puerto {port}.\n")
                self.text_area.see(END)
            except Exception as e:
                messagebox.showerror("Error", f"No se pudo conectar al puerto serial: {e}")

    def enviar_parametros(self):
        if not self.ser or not self.ser.is_open:
            messagebox.showerror("Error", "No está conectado al puerto serial.")
            return
        parametros = []
        campos = ['R', 'r', 'RPM_inicial_Disco', 'RPM_final_Disco', 'Coeficiente_de_friccion', 'Tiempo_total']
        parametro_nombre_dict = {
            'R': 'R',
            'r': 'r',
            'RPM_inicial_Disco': 'RPM_D_i',
            'RPM_final_Disco': 'RPM_D_f',
            'Coeficiente_de_friccion': 'mu',
            'Tiempo_total': 'T'
        }
        for campo in campos:
            attribute_name = campo
            entry = getattr(self, attribute_name + '_entry', None)
            if entry:
                valor = entry.get()
                if valor:
                    parametro_nombre = parametro_nombre_dict.get(campo, campo)
                    parametros.append(f"SET {parametro_nombre}={valor}\n")

        # Agregar comandos para la dirección de los motores
        dir_disco = self.dir_disco_var.get()
        dir_bola = self.dir_bola_var.get()
        parametros.append(f"SET DIR_DISCO={dir_disco}\n")
        parametros.append(f"SET DIR_BOLA={dir_bola}\n")

        if parametros:
            try:
                for param in parametros:
                    self.ser.write(param.encode())
                    time.sleep(0.1)
                self.text_area.insert(END, "Parámetros enviados.\n")
                self.text_area.see(END)

                # Actualizar el límite de muestras
                global limite_muestras
                limite_muestras = int(self.Limite_de_muestras_entry.get())
            except Exception as e:
                messagebox.showerror("Error", f"No se pudo enviar parámetros: {e}")
        else:
            messagebox.showwarning("Advertencia", "Ingrese al menos un parámetro.")

    def enviar_start(self):
        if not self.ser or not self.ser.is_open:
            messagebox.showerror("Error", "No está conectado al puerto serial.")
            return
        try:
            self.ser.write("START\n".encode())
            time.sleep(0.1)
            self.text_area.insert(END, "Ensayo iniciado.\n")
            self.text_area.see(END)
            # Iniciar hilo de lectura serial
            if not self.hilo_serial or not self.hilo_serial.is_alive():
                self.hilo_serial = threading.Thread(target=self.leer_serial)
                stop_event.clear()
                self.hilo_serial.start()
        except Exception as e:
            messagebox.showerror("Error", f"No se pudo iniciar el ensayo: {e}")

    def enviar_stop(self):
        if not self.ser or not self.ser.is_open:
            messagebox.showerror("Error", "No está conectado al puerto serial.")
            return
        try:
            self.ser.write("STOP\n".encode())
            time.sleep(0.1)
            self.text_area.insert(END, "Ensayo detenido.\n")
            self.text_area.see(END)
            stop_event.set()
        except Exception as e:
            messagebox.showerror("Error", f"No se pudo detener el ensayo: {e}")

    def reiniciar_datos(self):
        global datos_lista
        datos_lista.clear()
        self.tree.delete(*self.tree.get_children())
        self.fig.clear()
        self.canvas.draw()
        self.text_area.insert(END, "Datos y gráficas reiniciados.\n")
        self.text_area.see(END)
        # Reiniciar los valores actuales
        for label in self.valores_actuales.values():
            label.config(text="0.00")

    def guardar_datos(self):
        if datos_lista:
            # Preguntar al usuario si desea guardar todos los datos o un subconjunto
            respuesta = messagebox.askyesnocancel("Guardar Datos", "¿Desea guardar todos los datos? (Sí: todos, No: solo los últimos N, Cancelar: no guardar)")
            if respuesta is None:
                return  # Cancelado
            elif respuesta:
                datos = pd.DataFrame(datos_lista)
            else:
                # Pedir al usuario cuántas muestras desea guardar
                n = simpledialog.askinteger("Número de muestras", "¿Cuántas muestras recientes desea guardar?", minvalue=1, maxvalue=len(datos_lista))
                if n is None:
                    return  # Cancelado
                datos = pd.DataFrame(datos_lista[-n:])
            filename = filedialog.asksaveasfilename(defaultextension='.csv', filetypes=[('CSV Files', '*.csv')])
            if filename:
                datos.to_csv(filename, index=False)
                messagebox.showinfo("Información", f"Datos guardados en {filename}")
        else:
            messagebox.showwarning("Advertencia", "No hay datos para guardar.")

    def mostrar_ecuaciones(self):
        # Crear una nueva ventana
        ecuaciones_window = tk.Toplevel(self.root)
        ecuaciones_window.title("Ecuaciones Teóricas")
        ecuaciones_window.geometry("600x600")
        
        # Crear un Text widget con scrollbar
        text_widget = tk.Text(ecuaciones_window, wrap='word')
        text_widget.pack(side=LEFT, fill=BOTH, expand=True)
        
        scrollbar = ttkb.Scrollbar(ecuaciones_window, orient=VERTICAL, command=text_widget.yview)
        scrollbar.pack(side=RIGHT, fill=Y)
        text_widget.configure(yscrollcommand=scrollbar.set)
        
        # Contenido de las ecuaciones y explicaciones
        contenido = """
Ecuaciones Teóricas Utilizadas:

1. Velocidad Angular del Disco:
   ω_D = ω_{D_i} + α_D t
   - Donde:
     - ω_D: Velocidad angular del disco en rad/s
     - ω_{D_i}: Velocidad angular inicial del disco en rad/s
     - α_D: Aceleración angular del disco en rad/s²
     - t: Tiempo en segundos

2. Velocidad Angular de la Bola:
   ω_B = K ω_D
   - Donde:
     - ω_B: Velocidad angular de la bola en rad/s
     - K: Coeficiente que relaciona las velocidades (K = (1 - μ) * (R / r))
     - ω_D: Velocidad angular del disco en rad/s

3. Aceleración Angular de la Bola:
   α_B = K α_D
   - Donde:
     - α_B: Aceleración angular de la bola en rad/s²
     - α_D: Aceleración angular del disco en rad/s²

4. Frecuencia de Pulsos para el Motor:
   f_pulsos = (Pasos_por_Revolución / 2π) * ω
   - Donde:
     - f_pulsos: Frecuencia de pulsos en Hz
     - Pasos_por_Revolución: Número total de pasos por vuelta del motor (incluyendo microstepping)
     - ω: Velocidad angular en rad/s

5. Velocidad Lineal:
   v = ω * (Radio / 100)
   - Donde:
     - v: Velocidad lineal en m/s
     - ω: Velocidad angular en rad/s
     - Radio: Radio del disco o bola en cm

6. Tiempo en Alto del Pulso (PWM):
   T_pulse = (Duty_Cycle / Duty_Max) * (1 / f_pulsos)
   - Donde:
     - T_pulse: Tiempo en alto del pulso en segundos
     - Duty_Cycle: Valor actual del duty cycle
     - Duty_Max: Valor máximo del duty cycle (dependiendo de la resolución)
     - f_pulsos: Frecuencia de pulsos en Hz

Explicaciones:

- Las ecuaciones anteriores se utilizan para calcular las velocidades y aceleraciones tanto del disco como de la bola, así como para determinar las señales de control para los motores paso a paso.
- El coeficiente K toma en cuenta el deslizamiento entre el disco y la bola, ajustado por el coeficiente de fricción μ (mu).
- El cálculo de f_pulsos es esencial para generar las señales PWM que controlan los motores.

Nota: Los símbolos matemáticos se representan usando Unicode para compatibilidad.
"""

        # Insertar el contenido en el Text widget
        text_widget.insert(END, contenido)
        text_widget.configure(state='disabled')  # Hacer que el texto sea de solo lectura

    def leer_serial(self):
        try:
            while not stop_event.is_set():
                try:
                    line = self.ser.readline().decode('utf-8').strip()
                    if line:
                        # Mostrar mensajes y cálculos
                        self.text_area.insert(END, line + '\n')
                        self.text_area.see(END)
                    if line.startswith("Tiempo:"):
                        # Parsear la línea
                        partes = line.split('|')
                        tiempo = float(partes[0].split(':')[1].strip().split(' ')[0])
                        duty_D = float(partes[2].split(':')[1].strip().split('%')[0])
                        vel_D = float(partes[3].split(':')[1].strip().split(' ')[0])
                        duty_B = float(partes[5].split(':')[1].strip().split('%')[0])
                        vel_B = float(partes[6].split(':')[1].strip().split(' ')[0])
                        torque = float(partes[7].split(':')[1].strip().split(' ')[0])
                        T_pulse_D = float(partes[8].split(':')[1].strip().split(' ')[0])
                        T_pulse_B = float(partes[9].split(':')[1].strip().split(' ')[0])

                        # Agregar datos a la lista
                        nuevo_dato = {
                            'Tiempo': tiempo,
                            'Duty Disco': duty_D,
                            'Velocidad Disco': vel_D,
                            'Duty Bola': duty_B,
                            'Velocidad Bola': vel_B,
                            'Torque': torque,
                            'T_pulse_D': T_pulse_D,
                            'T_pulse_B': T_pulse_B
                        }
                        datos_lista.append(nuevo_dato)

                        # Si se excede el límite, eliminar la muestra más antigua
                        if len(datos_lista) > limite_muestras:
                            datos_lista.pop(0)

                        # Actualizar tabla
                        self.tree.insert('', END, values=(
                            tiempo, duty_D, vel_D, duty_B, vel_B, torque, T_pulse_D, T_pulse_B
                        ))

                        # Autoscroll hacia la última fila
                        self.tree.yview_moveto(1.0)

                    # Actualizar los valores actuales en las etiquetas
                    if datos_lista:
                        self.valores_actuales['Velocidad Disco (m/s)'].config(text=f"{datos_lista[-1]['Velocidad Disco']:.2f}")
                        self.valores_actuales['Velocidad Bola (m/s)'].config(text=f"{datos_lista[-1]['Velocidad Bola']:.2f}")
                        self.valores_actuales['Torque (kg)'].config(text=f"{datos_lista[-1]['Torque']:.2f}")
                        self.valores_actuales['Duty Disco (%)'].config(text=f"{datos_lista[-1]['Duty Disco']:.2f}")
                        self.valores_actuales['Duty Bola (%)'].config(text=f"{datos_lista[-1]['Duty Bola']:.2f}")

                except Exception as e:
                    print(f"Error al leer línea: {e}")
        except Exception as e:
            messagebox.showerror("Error", f"Error en la lectura serial: {e}")

    def actualizar_grafica(self, i):
        if datos_lista:
            datos = pd.DataFrame(datos_lista)
            self.fig.clear()
            ax1 = self.fig.add_subplot(3, 1, 1)
            ax2 = self.fig.add_subplot(3, 1, 2)
            ax3 = self.fig.add_subplot(3, 1, 3)

            # Graficar Velocidades
            ax1.plot(datos['Tiempo'], datos['Velocidad Disco'], label='Velocidad Disco (m/s)', color='blue')
            ax1.plot(datos['Tiempo'], datos['Velocidad Bola'], label='Velocidad Bola (m/s)', color='green')
            ax1.set_ylabel('Velocidad (m/s)')
            ax1.legend()
            ax1.grid(True)

            # Graficar Torque (Celda de Carga)
            ax2.plot(datos['Tiempo'], datos['Torque'], label='Torque (kg)', color='red')
            ax2.set_ylabel('Torque (kg)')
            ax2.legend()
            ax2.grid(True)

            # Graficar Duty Cycle
            ax3.plot(datos['Tiempo'], datos['Duty Disco'], label='Duty Disco (%)', color='cyan')
            ax3.plot(datos['Tiempo'], datos['Duty Bola'], label='Duty Bola (%)', color='magenta')
            ax3.set_xlabel('Tiempo (s)')
            ax3.set_ylabel('Duty Cycle (%)')
            ax3.legend()
            ax3.grid(True)

            self.fig.tight_layout()
            self.canvas.draw()

    def cerrar(self):
        stop_event.set()
        if self.hilo_serial and self.hilo_serial.is_alive():
            self.hilo_serial.join()
        if self.ser and self.ser.is_open:
            self.ser.close()
        self.root.destroy()

def main():
    root = ttkb.Window(themename='flatly')  # Puedes cambiar el tema aquí
    app = App(root)
    root.protocol("WM_DELETE_WINDOW", app.cerrar)
    root.mainloop()

if __name__ == '__main__':
    main()
