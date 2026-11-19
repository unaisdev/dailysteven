const MAGIC = new Uint32Array(new TextEncoder().encode('DAILYMAX-STATUS1').buffer);

export const PHASE = { EXPLORING: 0, BATTLE: 1, RESULT: 2, FINISHED: 3 };
export const RESULT = { NONE: 0, WON: 1, LOST: 2 };

export function createGameLink(emulator, seed) {
  let offset = -1;
  return function read() {
    const heap = emulator.getEmscriptenModule().HEAPU8;
    if (offset < 0) offset = findStatus(heap, seed >>> 0);
    if (offset < 0) return null;
    return { phase: heap[offset + 20], result: heap[offset + 21] };
  };
}

function findStatus(heap, seed) {
  const words = new Uint32Array(heap.buffer, heap.byteOffset, heap.byteLength >>> 2);
  for (let i = 0, end = words.length - 5; i < end; i++) {
    if (words[i] === MAGIC[0] && words[i + 1] === MAGIC[1] && words[i + 2] === MAGIC[2]
     && words[i + 3] === MAGIC[3] && words[i + 4] === seed) {
      return i * 4;
    }
  }
  return -1;
}
