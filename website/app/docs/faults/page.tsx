import { CodeBlock, DocLayout } from "../../components";

const fields = [
  ["c2s_latency_ms", "uint32", "Fixed client-to-server latency in milliseconds."],
  ["c2s_jitter_ms", "uint32", "Random client-to-server latency variance."],
  ["c2s_drop_rate", "float", "Client-to-server drop probability from 0 to 1."],
  ["c2s_throttle_kbps", "uint32", "Client-to-server bandwidth limit in kbps."],
  ["c2s_stall_prob", "float", "Client-to-server stall probability from 0 to 1."],
  ["s2c_latency_ms", "uint32", "Fixed server-to-client latency in milliseconds."],
  ["s2c_jitter_ms", "uint32", "Random server-to-client latency variance."],
  ["s2c_drop_rate", "float", "Server-to-client drop probability from 0 to 1."],
  ["s2c_throttle_kbps", "uint32", "Server-to-client bandwidth limit in kbps."],
  ["s2c_stall_prob", "float", "Server-to-client stall probability from 0 to 1."],
  ["latency_ms", "uint32", "Convenience shorthand applied to both directions."],
  ["drop_rate", "float", "Convenience shorthand applied to both directions."]
];

export default function Faults() {
  return (
    <DocLayout
      eyebrow="reference"
      title="Fault profiles"
      description="Profiles define how ShakyLine changes traffic in each direction of a TCP session."
    >
      <section className="docSection twoColumn">
        <div>
          <h2>Directional profile</h2>
          <p>
            Use c2s fields for client-to-server behavior and s2c fields for
            server-to-client behavior. This is useful when uploads and downloads
            need different failure conditions.
          </p>
        </div>
        <CodeBlock label="directional profile">
          {`{
  "c2s_latency_ms": 200,
  "c2s_jitter_ms": 30,
  "c2s_drop_rate": 0.05,
  "s2c_latency_ms": 100
}`}
        </CodeBlock>
      </section>

      <section className="docSection">
        <h2>Fields</h2>
        <div className="fieldTable">
          {fields.map(([field, type, description]) => (
            <div key={field}>
              <code>{field}</code>
              <span>{type}</span>
              <p>{description}</p>
            </div>
          ))}
        </div>
      </section>

      <section className="docSection">
        <h2>Common presets</h2>
        <div className="presetGrid">
          <CodeBlock label="slow api">
            {`{"latency_ms":300,"drop_rate":0.01}`}
          </CodeBlock>
          <CodeBlock label="bad upload">
            {`{"c2s_throttle_kbps":128,"c2s_drop_rate":0.04}`}
          </CodeBlock>
          <CodeBlock label="jittery realtime">
            {`{"c2s_jitter_ms":100,"s2c_jitter_ms":100}`}
          </CodeBlock>
        </div>
      </section>
    </DocLayout>
  );
}
