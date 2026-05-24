import { CodeBlock, DocLayout } from "../../components";

const endpoints = [
  ["GET", "/health", "Check whether the control server is alive."],
  ["GET", "/sessions", "List active proxy session ids and the current count."],
  ["GET", "/metrics", "Read Prometheus-compatible counters and gauges."],
  ["POST", "/profiles/{name}", "Create or update a named fault profile."],
  ["DELETE", "/profiles/{name}", "Remove a named profile and return traffic to baseline."]
];

export default function ApiDocs() {
  return (
    <DocLayout
      eyebrow="reference"
      title="Control API"
      description="ShakyLine is controlled through a small HTTP API exposed on the configured control port."
    >
      <section className="docSection">
        <h2>Endpoints</h2>
        <div className="apiTable">
          {endpoints.map(([method, path, description]) => (
            <div className="apiRow" key={`${method}-${path}`}>
              <span>{method}</span>
              <code>{path}</code>
              <p>{description}</p>
            </div>
          ))}
        </div>
      </section>

      <section className="docSection twoColumn">
        <div>
          <h2>Create a profile</h2>
          <p>
            Profiles are named. You can use names such as default, upload,
            checkout, or realtime to model different traffic scenarios.
          </p>
        </div>
        <CodeBlock label="post profile">
          {`curl -X POST http://localhost:9090/profiles/checkout \\
  -H "Content-Type: application/json" \\
  -d '{"c2s_latency_ms":250,"c2s_drop_rate":0.04}'`}
        </CodeBlock>
      </section>

      <section className="docSection twoColumn">
        <div>
          <h2>Inspect and reset</h2>
          <p>
            Use sessions and metrics while a test is running, then delete the
            profile when the scenario is complete.
          </p>
        </div>
        <CodeBlock label="observe">
          {`curl http://localhost:9090/health
curl http://localhost:9090/sessions
curl http://localhost:9090/metrics
curl -X DELETE http://localhost:9090/profiles/checkout`}
        </CodeBlock>
      </section>
    </DocLayout>
  );
}
