# Notas de Calibración

## Fecha: 2024-08-23

### Instrumentos Utilizados
- **Sensor FYAH**: Para la medición de torque.
- **Multímetro Digital**: Para verificar señales.
- **Fuente de Alimentación Regulable**: Usada para alimentar el circuito.

### Procedimiento de Calibración
1. **Preparación**: 
   - Conectar el sensor FYAH al ESP32 a través del conversor ADC.
   - Verificar que las lecturas de ADC correspondan con los valores de torque esperados.
   
2. **Recopilación de Datos**:
   - Aplicar diferentes fuerzas y registrar el valor ADC y el torque correspondiente.

3. **Ajuste de la Curva**:
   - Utilizar un ajuste lineal para correlacionar los valores del ADC con el torque.

4. **Verificación**:
   - Verificar la precisión de la calibración mediante una serie de pruebas adicionales.

### Resultados
- Los coeficientes de calibración se almacenaron en `coeficientes_calibración.txt`.
- La relación entre ADC y torque es lineal en el rango de operación.
