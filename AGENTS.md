# Notas para agentes

## Plan de continuación (leer primero)

1. [`docs/pending.md`](docs/pending.md) — **handoff**: qué está hecho, qué falta, siguiente ID, prompt para el compi
2. [`docs/gap-analisis-daily-steven.md`](docs/gap-analisis-daily-steven.md) — DEC-0 cerrado, matriz de gaps, backlog `GAP-*` por fases
3. [`docs/Daily_Steven_instrucciones_y_revision_independiente.md`](docs/Daily_Steven_instrucciones_y_revision_independiente.md) — spec DOCX + Anexo A (RI-01…16)
4. Informes de fase: [`fase0-gen3-informe.md`](docs/fase0-gen3-informe.md), [`fase1-daily-core-informe.md`](docs/fase1-daily-core-informe.md), [`fase2-steven-content-informe.md`](docs/fase2-steven-content-informe.md)

**Producto:** Daily Steven (`dailysteven:`). **Símbolos C / ROM / carpeta:** siguen `dailymaximo` / `DAILYMAX-*`.  
**No reabrir** OPEN-P* ni DEC-0 sin decisión nueva. Avanzar por fases 0→4 sin saltar puertas.

## Novedades y publicación (a mano)

Un push a main NO publica nada. El pack se publica solo lanzando a mano el workflow «Release content pack»
(o con una etiqueta `content-v*` puesta a propósito); crea un BORRADOR que se publica a mano.
Antes de lanzarlo, rellenar `content/NOVEDADES.md` (solo líneas `- …`, máx. 8, lo que el jugador note, en castellano).
Sin bullets, el workflow falla a propósito. Después se vacía con una PR normal (`scripts/clear-novedades.sh`).

## Compilar

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-wsl.ps1
# o: scripts/get-expansion.sh && scripts/build.sh / bash scripts/_docker_build.sh
```

→ `web/game/dailymaximo.gba.gz`. Base: HoL + overlay (`scripts/apply-dailymaximo.sh`).
