import pandas as pd
import matplotlib.pyplot as plt
import sys

# --- CONFIGURACIÓN ---
# 1. Escribe aquí el nombre de tu fichero de datos de entrada.
input_filename = "plots/weak_scaling_results.csv"
output_filename = 'weak_scaling_plot.png'
# --- FIN DE LA CONFIGURACIÓN ---


# 2. Cargar los datos desde el fichero .csv
try:
    # Intentamos leer el fichero CSV especificado arriba.
    df = pd.read_csv(input_filename)
    print(f"Fichero '{input_filename}' cargado exitosamente.")
except FileNotFoundError:
    # Si el fichero no existe, mostramos un error claro y salimos.
    print(f"--- ERROR: No se encontró el fichero '{input_filename}' ---")
    print("Asegúrate de que el script se está ejecutando en la misma carpeta que tu fichero .csv,")
    print("o de que el nombre del fichero está escrito correctamente en la sección de CONFIGURACIÓN.")
    sys.exit(1) # Termina el script con un código de error.

# Es muy importante ordenar los datos por el número de nodos para que la gráfica se muestre correctamente.
df_sorted = df.sort_values(by='Nodes').reset_index(drop=True)

# 3. Preparar la gráfica
plt.style.use('seaborn-v0_8-whitegrid')
fig, ax = plt.subplots(figsize=(10, 6))

# El tiempo ideal en escalado débil es el tiempo que tarda la ejecución con 1 nodo.
baseline_time = df_sorted.loc[df_sorted['Nodes'] == 1, 'Total_Time'].iloc[0]

# Convertimos los nodos a texto para que el eje X los trate como categorías separadas.
nodes_str = df_sorted['Nodes'].astype(str)

# 4. Dibujar los elementos de la gráfica
ax.bar(nodes_str, df_sorted['Total_Time'], color='dodgerblue', label='Tiempo de Ejecución Real')
ax.axhline(y=baseline_time, color='r', linestyle='--', label=f'Tiempo Ideal ({baseline_time:.2f}s)')

# 5. Añadir etiquetas, títulos y leyendas
ax.set_xlabel('Número de Nodos', fontsize=12)
ax.set_ylabel('Tiempo Total de Ejecución (segundos)', fontsize=12)
ax.set_title('Análisis de Escalabilidad Débil', fontsize=16)
ax.legend(fontsize=12)
ax.grid(axis='y', linestyle='--', alpha=0.7)
ax.set_ylim(bottom=0, top=max(df_sorted['Total_Time']) * 1.1)

# Añadimos etiquetas con el valor exacto encima de cada barra.
for i, val in enumerate(df_sorted['Total_Time']):
    ax.text(i, val + 10, f'{val:.2f}s', ha='center', va='bottom', fontsize=10)

# 6. Guardar la gráfica
plt.savefig(output_filename, bbox_inches='tight', dpi=150)
print(f"Gráfica guardada exitosamente como '{output_filename}'")