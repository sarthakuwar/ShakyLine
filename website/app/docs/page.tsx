import Link from "next/link";
import { CodeBlock, DocLayout } from "../components";

const docs = [
  ["Getting Started", "/docs/getting-started", "Install, run the proxy, and apply your first fault profile."],
  ["Use Cases", "/docs/use-cases", "Recipes for CI, local debugging, realtime apps, and incident rehearsal."],
  ["Control API", "/docs/api", "HTTP endpoints for profiles, health, sessions, and metrics."],
  ["Fault Profiles", "/docs/faults", "Fields for latency, jitter, drops, throttling, and stalls."]
] as const;

export default function Docs() {
  return (
    <DocLayout
      eyebrow="documentation"
      title="Operate ShakyLine with confidence."
      description="Use these guides to set up fault drills, automate scenarios, and understand the runtime control surface."
    >
      <div className="docCards large">
        {docs.map(([title, href, copy]) => (
          <Link href={href} key={href}>
            <h3>{title}</h3>
            <p>{copy}</p>
          </Link>
        ))}
      </div>
      <CodeBlock label="common loop">
        {`# 1. Start proxy
shakyline --listen 0.0.0.0:8080 --upstream api.example.com:443

# 2. Apply a profile
curl -X POST http://localhost:9090/profiles/default \\
  -H "Content-Type: application/json" \\
  -d '{"latency_ms":150,"drop_rate":0.02}'

# 3. Inspect status
curl http://localhost:9090/sessions
curl http://localhost:9090/metrics

# 4. Reset
curl -X DELETE http://localhost:9090/profiles/default`}
      </CodeBlock>
    </DocLayout>
  );
}
