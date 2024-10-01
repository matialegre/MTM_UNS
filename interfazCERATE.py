import sys
import threading
import serial
import serial.tools.list_ports
import time
import csv
from PyQt5 import QtCore, QtGui, QtWidgets
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
import matplotlib.pyplot as plt

class MiniTractionMachine(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Mini Traction Machine")
        self.setGeometry(100, 100, 1200, 800)

        self.serial_port = None
        self.is_connected = False
        self.data_thread = None
        self.running = False

        self.rpm1_data = []
        self.rpm2_data = []
        self.torque_data = []
        self.time_data = []
        self.start_time = time.time()

        self.setup_ui()
        self.setup_plots()
        self.setup_connections()
        self.refresh_ports()

        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update_graph)
        self.timer.start(500)

    def setup_ui(self):
        # Ventana principal y layout
        central_widget = QtWidgets.QWidget()
        main_layout = QtWidgets.QVBoxLayout(central_widget)

        # Título
        title_label = QtWidgets.QLabel("Mini Traction Machine")
        title_font = QtGui.QFont()
        title_font.setPointSize(18)
        title_font.setBold(True)
        title_label.setFont(title_font)
        title_label.setAlignment(QtCore.Qt.AlignCenter)
        main_layout.addWidget(title_label)

        # Información
        info_label = QtWidgets.QLabel("Control de motores paso a paso para ensayos de tracción.")
        info_label.setAlignment(QtCore.Qt.AlignCenter)
        main_layout.addWidget(info_label)

        # Conexión Serial
        serial_layout = QtWidgets.QHBoxLayout()
        port_label = QtWidgets.QLabel("Puerto Serial:")
        self.port_combo = QtWidgets.QComboBox()
        self.refresh_button = QtWidgets.QPushButton("Actualizar Puertos")
        self.connect_button = QtWidgets.QPushButton("Conectar")
        serial_layout.addWidget(port_label)
        serial_layout.addWidget(self.port_combo)
        serial_layout.addWidget(self.refresh_button)
        serial_layout.addWidget(self.connect_button)
        main_layout.addLayout(serial_layout)

        # Layout principal para los controles de motores
        motors_layout = QtWidgets.QHBoxLayout()

        # Diccionario para almacenar los widgets de cada motor
        self.motor_widgets = {}

        for motor_number in [1, 2]:
            # Frame para cada motor
            motor_frame = QtWidgets.QFrame()
            motor_layout = QtWidgets.QVBoxLayout(motor_frame)

            # Título del motor
            motor_label = QtWidgets.QLabel(f"Motor {motor_number}")
            motor_label.setFont(title_font)
            motor_label.setAlignment(QtCore.Qt.AlignCenter)
            motor_layout.addWidget(motor_label)

            # Sección para configurar microstepping y radio
            config_group = QtWidgets.QGroupBox("Configuración")
            config_layout = QtWidgets.QFormLayout(config_group)
            microsteps_input = QtWidgets.QSpinBox()
            microsteps_input.setRange(1, 6400)
            microsteps_input.setValue(6400)
            radius_input = QtWidgets.QDoubleSpinBox()
            radius_input.setRange(0.0, 1000.0)
            radius_input.setValue(10.0)
            radius_input.setSuffix(" mm")
            send_config_button = QtWidgets.QPushButton("Enviar Configuración")
            send_config_button.clicked.connect(lambda _, m=motor_number: self.send_config(m))
            config_layout.addRow("Microstepping:", microsteps_input)
            config_layout.addRow("Radio:", radius_input)
            config_layout.addWidget(send_config_button)
            motor_layout.addWidget(config_group)

            # Sección 1: Mover Grados a una RPM
            group_deg = QtWidgets.QGroupBox("Mover Grados a una RPM")
            deg_layout = QtWidgets.QFormLayout(group_deg)
            deg_input = QtWidgets.QSpinBox()
            deg_input.setRange(1, 360)
            deg_input.setValue(90)
            rpm_deg_input = QtWidgets.QSpinBox()
            rpm_deg_input.setRange(1, 10000)
            rpm_deg_input.setValue(100)
            deg_send_button = QtWidgets.QPushButton("Enviar")
            deg_send_button.clicked.connect(lambda _, m=motor_number: self.send_deg_command(m))
            deg_layout.addRow("Grados:", deg_input)
            deg_layout.addRow("RPM:", rpm_deg_input)
            deg_layout.addWidget(deg_send_button)
            motor_layout.addWidget(group_deg)

            # Sección 2: Establecer RPM Constante
            group_rpm = QtWidgets.QGroupBox("Establecer RPM Constante")
            rpm_layout = QtWidgets.QFormLayout(group_rpm)
            rpm_input = QtWidgets.QSpinBox()
            rpm_input.setRange(1, 10000)
            rpm_input.setValue(100)
            rpm_send_button = QtWidgets.QPushButton("Enviar")
            rpm_send_button.clicked.connect(lambda _, m=motor_number: self.send_rpm_command(m))
            rpm_layout.addRow("RPM:", rpm_input)
            rpm_layout.addWidget(rpm_send_button)
            motor_layout.addWidget(group_rpm)

            # Sección 3: Aceleración entre dos RPM
            group_accel = QtWidgets.QGroupBox("Aceleración entre dos RPM")
            accel_layout = QtWidgets.QFormLayout(group_accel)
            rpm_init_input = QtWidgets.QSpinBox()
            rpm_init_input.setRange(0, 10000)
            rpm_init_input.setValue(100)
            rpm_final_input = QtWidgets.QSpinBox()
            rpm_final_input.setRange(0, 10000)
            rpm_final_input.setValue(500)
            time_input = QtWidgets.QDoubleSpinBox()
            time_input.setRange(0.1, 1000)
            time_input.setValue(5.0)
            time_input.setSingleStep(0.1)
            accel_linear_button = QtWidgets.QPushButton("Aceleración Lineal")
            accel_linear_button.clicked.connect(lambda _, m=motor_number: self.send_accel_command(m, "linear"))
            accel_scurve_button = QtWidgets.QPushButton("Aceleración Curva S")
            accel_scurve_button.clicked.connect(lambda _, m=motor_number: self.send_accel_command(m, "s-curve"))
            accel_layout.addRow("RPM Inicial:", rpm_init_input)
            accel_layout.addRow("RPM Final:", rpm_final_input)
            accel_layout.addRow("Tiempo (s):", time_input)
            accel_buttons_layout = QtWidgets.QHBoxLayout()
            accel_buttons_layout.addWidget(accel_linear_button)
            accel_buttons_layout.addWidget(accel_scurve_button)
            accel_layout.addRow(accel_buttons_layout)
            motor_layout.addWidget(group_accel)

            # Botón para detener el motor
            stop_button = QtWidgets.QPushButton("Detener Motor")
            stop_button.clicked.connect(lambda _, m=motor_number: self.send_stop_command(m))
            motor_layout.addWidget(stop_button)

            # Etiqueta para mostrar velocidad lineal
            linear_speed_label = QtWidgets.QLabel("Velocidad Lineal: 0.00 m/s")
            motor_layout.addWidget(linear_speed_label)

            # Agregar el frame al layout de motores
            motors_layout.addWidget(motor_frame)

            # Almacenar referencias a los widgets
            self.motor_widgets[motor_number] = {
                'microsteps_input': microsteps_input,
                'radius_input': radius_input,
                'deg_input': deg_input,
                'rpm_deg_input': rpm_deg_input,
                'rpm_input': rpm_input,
                'rpm_init_input': rpm_init_input,
                'rpm_final_input': rpm_final_input,
                'time_input': time_input,
                'linear_speed_label': linear_speed_label
            }

        main_layout.addLayout(motors_layout)

        # Área de Mensajes
        messages_label = QtWidgets.QLabel("Mensajes:")
        main_layout.addWidget(messages_label)
        self.message_area = QtWidgets.QTextEdit()
        self.message_area.setReadOnly(True)
        main_layout.addWidget(self.message_area)

        # Gráfica
        graph_group = QtWidgets.QGroupBox("Gráfica de Torque, RPM y Velocidad Lineal")
        graph_layout = QtWidgets.QVBoxLayout(graph_group)
        self.figure = plt.figure()
        self.canvas = FigureCanvas(self.figure)
        graph_layout.addWidget(self.canvas)
        main_layout.addWidget(graph_group)

        # Establecer el widget central
        self.setCentralWidget(central_widget)

    def setup_plots(self):
        pass  # La gráfica ya se configuró en setup_ui

    def setup_connections(self):
        self.refresh_button.clicked.connect(self.refresh_ports)
        self.connect_button.clicked.connect(self.connect_serial)

    def refresh_ports(self):
        self.port_combo.clear()
        ports = serial.tools.list_ports.comports()
        for port in ports:
            self.port_combo.addItem(port.device)

    def connect_serial(self):
        if not self.is_connected:
            port_name = self.port_combo.currentText()
            try:
                self.serial_port = serial.Serial(port_name, 115200, timeout=1)
                self.is_connected = True
                self.running = True
                self.data_thread = threading.Thread(target=self.read_serial_data)
                self.data_thread.start()
                QtWidgets.QMessageBox.information(self, "Conexión Exitosa", f"Conectado a {port_name}")
                self.connect_button.setText("Desconectar")
            except Exception as e:
                QtWidgets.QMessageBox.critical(self, "Error", f"No se pudo conectar al puerto {port_name}\n{str(e)}")
        else:
            self.running = False
            time.sleep(1)
            self.serial_port.close()
            self.is_connected = False
            QtWidgets.QMessageBox.information(self, "Desconectado", "Conexión cerrada.")
            self.connect_button.setText("Conectar")

    def send_config(self, motor_number):
        if self.is_connected:
            microsteps = self.motor_widgets[motor_number]['microsteps_input'].value()
            command = f"MSPR{microsteps}"
            self.serial_port.write((command + '\n').encode())
            self.message_area.append(f"Enviado al Motor {motor_number}: {command}")
        else:
            QtWidgets.QMessageBox.warning(self, "Desconectado", "Por favor, conecte el puerto serial primero.")

    def send_deg_command(self, motor_number):
        if self.is_connected:
            deg = self.motor_widgets[motor_number]['deg_input'].value()
            rpm = self.motor_widgets[motor_number]['rpm_deg_input'].value()
            command = f"M{motor_number}{deg},{rpm}"
            self.serial_port.write((command + '\n').encode())
            self.message_area.append(f"Enviado al Motor {motor_number}: {command}")
        else:
            QtWidgets.QMessageBox.warning(self, "Desconectado", "Por favor, conecte el puerto serial primero.")

    def send_rpm_command(self, motor_number):
        if self.is_connected:
            rpm = self.motor_widgets[motor_number]['rpm_input'].value()
            command = f"S{motor_number}{rpm}"
            self.serial_port.write((command + '\n').encode())
            self.message_area.append(f"Enviado al Motor {motor_number}: {command}")
        else:
            QtWidgets.QMessageBox.warning(self, "Desconectado", "Por favor, conecte el puerto serial primero.")

    def send_accel_command(self, motor_number, mode):
        if self.is_connected:
            rpm_init = self.motor_widgets[motor_number]['rpm_init_input'].value()
            rpm_final = self.motor_widgets[motor_number]['rpm_final_input'].value()
            time_sec = self.motor_widgets[motor_number]['time_input'].value()
            if mode == "linear":
                command = f"AL{motor_number}{rpm_init},{rpm_final},{time_sec}"
            elif mode == "s-curve":
                command = f"AS{motor_number}{rpm_init},{rpm_final},{time_sec}"
            else:
                return
            self.serial_port.write((command + '\n').encode())
            self.message_area.append(f"Enviado al Motor {motor_number}: {command}")
        else:
            QtWidgets.QMessageBox.warning(self, "Desconectado", "Por favor, conecte el puerto serial primero.")

    def send_stop_command(self, motor_number):
        if self.is_connected:
            command = f"P{motor_number}"
            self.serial_port.write((command + '\n').encode())
            self.message_area.append(f"Enviado al Motor {motor_number}: {command}")
        else:
            QtWidgets.QMessageBox.warning(self, "Desconectado", "Por favor, conecte el puerto serial primero.")

    def read_serial_data(self):
        while self.running:
            try:
                line = self.serial_port.readline().decode().strip()
                if line:
                    self.message_area.append(f"Recibido: {line}")
                    if line.startswith("Torque:"):
                        self.parse_data(line)
            except Exception as e:
                print(f"Error leyendo datos: {e}")

    def parse_data(self, data_line):
        try:
            data_parts = data_line.split(',')
            torque_str = data_parts[0].split(':')[1]
            rpm1_str = data_parts[1].split(':')[1]
            rpm2_str = data_parts[2].split(':')[1]

            torque_value = float(torque_str) + 1.0  # Agregar +1 al torque
            rpm1_value = float(rpm1_str)
            rpm2_value = float(rpm2_str)
            timestamp = time.time() - self.start_time

            self.torque_data.append(torque_value)
            self.rpm1_data.append(rpm1_value)
            self.rpm2_data.append(rpm2_value)
            self.time_data.append(timestamp)

            # Calcular velocidad lineal para cada motor
            for motor_number in [1, 2]:
                rpm_value = rpm1_value if motor_number == 1 else rpm2_value
                radius_mm = self.motor_widgets[motor_number]['radius_input'].value()
                radius_m = radius_mm / 1000.0  # Convertir a metros
                linear_speed = (rpm_value * 2 * 3.1416 * radius_m) / 60  # m/s
                self.motor_widgets[motor_number]['linear_speed_label'].setText(f"Velocidad Lineal: {linear_speed:.2f} m/s")

        except Exception as e:
            print(f"Error al parsear datos: {e}")

    def update_graph(self):
        if self.time_data:
            self.figure.clear()
            ax = self.figure.add_subplot(111)
            ax.plot(self.time_data, self.torque_data, label="Torque (Nm)", color='blue')
            ax.plot(self.time_data, self.rpm1_data, label="RPM Motor 1", color='red')
            ax.plot(self.time_data, self.rpm2_data, label="RPM Motor 2", color='green')
            ax.set_xlabel("Tiempo (s)")
            ax.set_ylabel("Valor")
            ax.set_title("Torque y RPM vs Tiempo")
            ax.legend()
            self.canvas.draw()

    def closeEvent(self, event):
        if self.is_connected:
            self.running = False
            time.sleep(1)
            self.serial_port.close()
        event.accept()

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    window = MiniTractionMachine()
    window.show()
    sys.exit(app.exec_())
