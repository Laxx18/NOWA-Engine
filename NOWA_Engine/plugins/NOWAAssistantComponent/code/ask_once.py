#!/usr/bin/env python3
import argparse
import json
import os
import sys
import time
import traceback

def log_path_next_to_script() -> str:
    try:
        script_dir = os.path.dirname(os.path.abspath(__file__))
    except Exception:
        script_dir = os.getcwd()
    return os.path.join(script_dir, "ask_once_last.log")

LOG_PATH = log_path_next_to_script()

def write_log(text: str) -> None:
    # Always append so we can see history across runs
    with open(LOG_PATH, "a", encoding="utf-8", errors="replace") as f:
        f.write(text)
        if not text.endswith("\n"):
            f.write("\n")

def banner(title: str) -> None:
    write_log("\n" + "=" * 70)
    write_log(title)
    write_log("=" * 70)

def main() -> int:
    banner("ASK_ONCE START")
    write_log(f"time: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    write_log(f"sys.executable: {sys.executable}")
    write_log(f"python: {sys.version}")
    write_log(f"cwd: {os.getcwd()}")
    write_log(f"argv: {sys.argv}")

    parser = argparse.ArgumentParser()
    parser.add_argument("--question", required=True)
    parser.add_argument("--k", type=int, default=6)
    parser.add_argument("--qdrant", default="http://localhost:6333")
    parser.add_argument("--ollama", default="http://localhost:11434")
    parser.add_argument("--collection_text", default="nowa_rag_text")
    args = parser.parse_args()

    write_log(f"question: {args.question}")
    write_log(f"k: {args.k}")
    write_log(f"qdrant: {args.qdrant}")
    write_log(f"ollama: {args.ollama}")
    write_log(f"collection_text: {args.collection_text}")

    import requests
    from qdrant_client import QdrantClient

    # ---- Health checks
    banner("HEALTH CHECKS")

    try:
        r = requests.get(args.qdrant.rstrip("/") + "/healthz", timeout=10)
        write_log(f"Qdrant /healthz: {r.status_code} {r.text.strip()[:200]}")
    except Exception as e:
        write_log(f"Qdrant /healthz ERROR: {repr(e)}")

    try:
        r = requests.get(args.ollama.rstrip("/") + "/api/tags", timeout=10)
        write_log(f"Ollama /api/tags: {r.status_code} {r.text.strip()[:200]}")
    except Exception as e:
        write_log(f"Ollama /api/tags ERROR: {repr(e)}")

    # ---- Connect Qdrant and list collections
    banner("QDRANT COLLECTIONS")
    q = QdrantClient(url=args.qdrant)

    try:
        cols = q.get_collections()
        names = [c.name for c in cols.collections]
        write_log("collections: " + ", ".join(names))
    except Exception as e:
        write_log("get_collections ERROR: " + repr(e))

    # ---- Embed question using Ollama
    banner("EMBED QUERY (OLLAMA)")
    emb = requests.post(
        args.ollama.rstrip("/") + "/api/embeddings",
        json={"model": "nomic-embed-text", "prompt": args.question},
        timeout=120
    )
    write_log(f"embeddings status: {emb.status_code}")
    emb.raise_for_status()
    vec = emb.json().get("embedding", None)
    if not vec or not isinstance(vec, list):
        raise RuntimeError("No embedding returned from Ollama.")
    write_log(f"embedding dim: {len(vec)}")

    # ---- Query Qdrant (new API)
    banner("QDRANT QUERY")
    res = q.query_points(
        collection_name=args.collection_text,
        query=vec,
        limit=max(1, min(20, args.k)),
        with_payload=True
    )

    write_log(f"hits: {len(res.points)}")
    for i, p in enumerate(res.points):
        payload = p.payload or {}
        write_log(f"{i+1}. score={getattr(p,'score',None)} relpath={payload.get('relpath')} chunk={payload.get('chunk_index')}")
        ex = payload.get("excerpt")
        if ex:
            write_log("   excerpt: " + str(ex).replace("\n", " ")[:220])

    # ---- Print answer to stdout (so your engine UI can show it)
    print("Top hits:")
    for p in res.points:
        payload = p.payload or {}
        print(f"- {payload.get('relpath')}  (chunk {payload.get('chunk_index')})")

    write_log("ASK_ONCE OK")
    return 0

if __name__ == "__main__":
    try:
        rc = main()
        sys.exit(rc)
    except SystemExit:
        raise
    except Exception:
        banner("ASK_ONCE FAILED")
        write_log(traceback.format_exc())
        # Also print something so your UI shows it even if logging fails later
        print("ERROR: ask_once failed. See log:", LOG_PATH)
        sys.exit(1)