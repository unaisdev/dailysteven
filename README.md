# Daily Steven

Content pack for [Hortelano's Daily](https://github.com/unaisdev/hortelanos-daily): one shared doubles battle a day against **Steven** and **Aura** or **Blasco**.

The repo folder and the ROM magic strings stay `dailymaximo` / `DAILYMAX-*`. The product name, the daily seed (`dailysteven:`) and the names spoken in battle are Daily Steven / STEVEN / AURA / BLASCO.

Built on [Hall of Legends](https://github.com/amoi-builds/halloflegends) / [pokeemerald-expansion](https://github.com/rh-hideout/pokeemerald-expansion), with the same browser + mGBA flow as Daily Red / Daily Gold.

## Play locally

```sh
scripts/get-expansion.sh   # pokeemerald-expansion + HoL + Daily Máximo overlay + castellano (locale/es)
scripts/build.sh           # or on Windows: bash scripts/_docker_build.sh
scripts/fetch-vendor.sh
node scripts/serve.mjs
```

Castellano uses the same Acimut `pret_europe` overlays as [Daily Gold](https://github.com/unaisdev/dailygold) (`locale/es/` + `scripts/apply-locale.sh`).

On Windows without a native ARM toolchain, use Docker Desktop:

```sh
bash scripts/_docker_build.sh
bash scripts/_package_web.sh   # if build ran make directly
scripts/fetch-vendor.sh
node scripts/serve.mjs
```

Open http://localhost:8080. Practice with `/?seed=1` (Aura) and `/?seed=2` (Blasco).

## How it works

1. The site hashes `dailysteven:YYYY-MM-DD` → seed and injects `DAILYMAX-CONF-V1` into the ROM.
2. The game generates a level-100 team of six and warps onto the champion stage (no door walk-in). Bruno’s room and the door cinematic are the target scene; they are not in this build yet.
3. The daily fight starts immediately: you alone vs Steven’s 3 + Aura or Blasco (`seed & 1`)’s 3. Each partner has one fixed team for now.
4. Result is written to `DAILYMAX-STATUS1` in RAM for the site.

See `docs/status-al-entrar.md` for optional status-on-entry notes.

Spec gap analysis (Daily Steven DOCX + independent review vs this repo): [`docs/gap-analisis-daily-steven.md`](docs/gap-analisis-daily-steven.md). Full spec + Anexo A: [`docs/Daily_Steven_instrucciones_y_revision_independiente.md`](docs/Daily_Steven_instrucciones_y_revision_independiente.md).

## Disclaimer

Fan project, not affiliated with Nintendo, Game Freak or The Pokémon Company.
