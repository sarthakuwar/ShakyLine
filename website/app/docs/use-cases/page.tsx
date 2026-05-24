import { CodeBlock, DocLayout } from "../../components";

const scenarios = [
  {
    title: "API timeout and retry testing",
    copy: "Add latency and packet loss between a client and upstream API to confirm retry budgets, timeout messages, and fallback paths.",
    command: `curl -X POST http://localhost:9090/profiles/default \\
  -H "Content-Type: application/json" \\
  -d '{"latency_ms":500,"drop_rate":0.03}'`
  },
  {
    title: "Upload path degradation",
    copy: "Apply faults only from client to server to test file uploads, telemetry ingestion, or write-heavy APIs.",
    command: `curl -X POST http://localhost:9090/profiles/upload \\
  -H "Content-Type: application/json" \\
  -d '{"c2s_throttle_kbps":128,"c2s_jitter_ms":80}'`
  },
  {
    title: "Realtime connection stability",
    copy: "Introduce jitter and stalls while websocket-like TCP traffic stays connected for long periods.",
    command: `curl -X POST http://localhost:9090/profiles/realtime \\
  -H "Content-Type: application/json" \\
  -d '{"c2s_jitter_ms":120,"s2c_jitter_ms":120,"c2s_stall_prob":0.02}'`
  },
  {
    title: "CI regression scenario",
    copy: "Use a fixed seed and a named profile to make a failure mode reproducible across repeated test runs.",
    command: `shakyline --listen 127.0.0.1:8080 \\
  --upstream 127.0.0.1:9000 \\
  --seed 4242`
  }
];

export default function UseCases() {
  return (
    <DocLayout
      eyebrow="playbooks"
      title="Use cases"
      description="Practical fault-injection scenarios you can run locally, in staging, or inside CI."
    >
      <div className="scenarioList">
        {scenarios.map((scenario) => (
          <article key={scenario.title}>
            <div>
              <h2>{scenario.title}</h2>
              <p>{scenario.copy}</p>
            </div>
            <CodeBlock label="recipe">{scenario.command}</CodeBlock>
          </article>
        ))}
      </div>
    </DocLayout>
  );
}
