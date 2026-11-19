# Notas para agentes

## Plan de continuación (leer primero)

El backlog ejecutable y las decisiones cerradas están en:

1. [`docs/gap-analisis-daily-steven.md`](docs/gap-analisis-daily-steven.md) — DEC-0 cerrado, matriz de gaps, backlog `GAP-*` por fases, prompt mínimo para subagentes
2. [`docs/Daily_Steven_instrucciones_y_revision_independiente.md`](docs/Daily_Steven_instrucciones_y_revision_independiente.md) — spec DOCX + Anexo A (RI-01…16)
3. [`docs/fase0-gen3-informe.md`](docs/fase0-gen3-informe.md) — informe Fase 0 ya hecho

**Producto:** Daily Steven (`dailysteven:`). **Símbolos C / ROM / carpeta:** siguen `dailymaximo` / `DAILYMAX-*`.  
**No reabrir** OPEN-P* ni DEC-0 sin decisión nueva. Avanzar por fases 0→4 sin saltar puertas.

## Obligatorio: Novedades antes de push a main

Rellenar `content/NOVEDADES.md` (solo líneas `- …`) con lo que el jugador note.
Sin bullets, el Release falla a propósito.

## Compilar

```sh
rm -rf pokeemerald-expansion && scripts/get-expansion.sh
scripts/build.sh   # → web/game/dailymaximo.gba.gz
```

Base: Hall of Legends patch + overlay en `overlay/` aplicado por `scripts/apply-dailymaximo.sh`.
