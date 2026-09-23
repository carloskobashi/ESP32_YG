# Mapa Modbus usado

Offset 0 = registro Modbus 30001.

Bloques leidos:

- 0..78      -> 30001..30079
- 80..110    -> 30081..30111
- 200..224   -> 30201..30225
- 234..269   -> 30235..30270
- 334..394   -> 30335..30395

El ultimo bloque termina exactamente en 30395, el ultimo registro utilizado por este firmware.
