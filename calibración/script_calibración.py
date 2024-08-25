import pandas as pd
import matplotlib.pyplot as plt

# Cargar datos de calibración
datos = pd.read_csv('datos_calibración.csv')

# Realizar ajuste lineal de la curva de calibración
from numpy import polyfit
coeficientes = polyfit(datos['Valor ADC'], datos['Torque (Nm)'], 1)

# Visualizar curva de calibración
plt.plot(datos['Valor ADC'], datos['Torque (Nm)'], 'o', label='Datos Experimentales')
plt.plot(datos['Valor ADC'], coeficientes[0]*datos['Valor ADC'] + coeficientes[1], label='Ajuste Lineal')
plt.xlabel('Valor ADC')
plt.ylabel('Torque (Nm)')
plt.title('Curva de Calibración del Sensor de Torque')
plt.legend()
plt.grid(True)
plt.show()

# Guardar coeficientes de calibración
with open('coeficientes_calibración.txt', 'w') as f:
    f.write(f"Coeficiente 1: {coeficientes[0]}\n")
    f.write(f"Coeficiente 2: {coeficientes[1]}\n")
