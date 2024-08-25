import serial
import time

class ControladorMotor:
    def __init__(self, puerto='/dev/ttyUSB0', baudrate=115200):
        try:
            self.serial_port = serial.Serial(puerto, baudrate, timeout=1)
        except serial.SerialException as e:
            print(f"Error al abrir el puerto serial: {e}")
            self.serial_port = None
    
    def enviar_comando_motor1(self, rpm):
        if self.serial_port:
            comando = f'SET_RPM:{rpm}\n'
            self.serial_port.write(comando.encode())
    
    def obtener_datos_torque(self):
        if self.serial_port:
            self.serial_port.write(b'GET_TORQUE\n')
            respuesta = self.serial_port.readline().decode().strip()
            if respuesta.startswith("TORQUE"):
                try:
                    torque = float(respuesta.split(':')[1])
                    tiempo_actual = time.time()
                    return {'tiempo': tiempo_actual, 'torque': torque}
                except ValueError:
                    print("Error al interpretar los datos de torque.")
            else:
                print("Datos de torque no válidos recibidos.")
        return {'tiempo': 0, 'torque': 0}
