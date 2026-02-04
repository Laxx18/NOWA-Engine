import argparse
import requests
from qdrant_client import QdrantClient

OLLAMA = "http://localhost:11434"
QDRANT = "http://localhost:6333"
EMBED_MODEL = "nomic-embed-text"

COLLECTION_TEXT = "nowa_rag_text"
COLLECTION_ASSETS = "nowa_asset_catalog"

def embed(text: str):
    r = requests.post(
        f"{OLLAMA}/api/embeddings",
        json={"model": EMBED_MODEL, "prompt": text},
        timeout=120
    )
    r.raise_for_status()
    return r.json()["embedding"]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--question", required=True)
    ap.add_argument("--k", type=int, default=6)
    args = ap.parse_args()

    client = QdrantClient(url=QDRANT)

    q = args.question.strip()
    if not q:
        print("Empty question.")
        return

    qvec = embed(q)

    print("QUESTION:", q)
    print("\n=== TEXT MATCHES ===")
    hits = client.search(collection_name=COLLECTION_TEXT, query_vector=qvec, limit=args.k, with_payload=True)
    for h in hits:
        p = h.payload or {}
        rel = p.get("relpath") or p.get("path")
        idx = p.get("chunk_index")
        ex = (p.get("excerpt") or "").replace("\n", " ")
        print(f"- score {h.score:.4f} | {rel} | chunk {idx}")
        if ex:
            print("  ", ex[:220] + ("..." if len(ex) > 220 else ""))

    print("\n=== ASSET MATCHES ===")
    hits2 = client.search(collection_name=COLLECTION_ASSETS, query_vector=qvec, limit=args.k, with_payload=True)
    for h in hits2:
        p = h.payload or {}
        rel = p.get("relpath") or p.get("path")
        print(f"- score {h.score:.4f} | {rel}")

if __name__ == "__main__":
    main()
