import Link from "next/link";
import { CodeBlock, ExternalLink, Footer, Header, links } from "./components";

const useCases = [
  ["Resilience testing", "Verify retries, timeouts, and degraded-mode behavior before production traffic finds the weak path."],
  ["CI failure drills", "Run repeatable latency and packet-loss scenarios as part of integration tests."],
  ["Streaming and realtime apps", "Simulate jitter, slow upstreams, and asymmetric traffic for sockets and long-lived TCP flows."],
  ["Incident rehearsal", "Practice service-owner playbooks against controlled network failure instead of synthetic dashboards."]
];

const workflow = [
  ["1", "Start the proxy", "Choose a local listen port and an upstream host."],
  ["2", "Apply a profile", "POST a named fault profile to the control API."],
  ["3", "Run your client", "Send normal application traffic through ShakyLine."],
  ["4", "Reset traffic", "DELETE the profile when the scenario is done."]
];

export default function Home() {
  return (
    <main>
      <Header />

      <section className="hero" id="top">
        <div className="heroCopy">
          <div className="statusChip">
            <span className="pulse" />
            v1.0.0 stable
          </div>
          <h1>Break the network before the network breaks you.</h1>
          <p>
            ShakyLine is a transparent TCP fault-injection proxy for teams that
            need to test latency, jitter, packet loss, throttling, and stalls
            against real application traffic.
          </p>
          <div className="heroActions">
            <ExternalLink className="primaryButton" href={links.windowsInstaller}>
              Download for Windows
            </ExternalLink>
            <Link className="secondaryButton" href="/docs/getting-started">
              Read the docs
            </Link>
          </div>
        </div>

        <div className="heroPanel" aria-label="Quick start terminal preview">
          <CodeBlock label="run a fault drill">
            {`shakyline --listen 0.0.0.0:8080 \\
  --upstream api.example.com:443 \\
  --control 9090

curl -X POST http://localhost:9090/profiles/default \\
  -H "Content-Type: application/json" \\
  -d '{"c2s_latency_ms":200,"c2s_drop_rate":0.05}'`}
          </CodeBlock>
        </div>
      </section>

      <section className="stats" aria-label="Project status">
        <div>
          <span>release</span>
          <strong>v1.0.0</strong>
        </div>
        <div>
          <span>runtime</span>
          <strong>TCP proxy</strong>
        </div>
        <div>
          <span>control api</span>
          <strong>HTTP</strong>
        </div>
        <div>
          <span>download</span>
          <strong>Windows x64</strong>
        </div>
      </section>

      <section className="section" id="download">
        <div className="sectionHeader">
          <span className="eyebrow">download</span>
          <h2>Get ShakyLine running.</h2>
          <p>
            Use the Windows installer for the fastest setup, grab the portable
            ZIP for manual installs, or build from source on Linux and macOS.
          </p>
        </div>

        <div className="downloadGrid three">
          <article className="downloadCard accent">
            <div>
              <span className="platform">Recommended</span>
              <h3>Windows installer</h3>
              <p>Install ShakyLine and open a new terminal to run `shakyline` from PATH.</p>
            </div>
            <ExternalLink className="cardButton" href={links.windowsInstaller}>
              Download .exe
            </ExternalLink>
          </article>
          <article className="downloadCard">
            <div>
              <span className="platform">Portable</span>
              <h3>Windows ZIP</h3>
              <p>Extract the archive and run the binary from the folder you choose.</p>
            </div>
            <ExternalLink className="cardButton secondary" href={links.windowsZip}>
              Download .zip
            </ExternalLink>
          </article>
          <article className="downloadCard">
            <div>
              <span className="platform">Linux and macOS</span>
              <h3>Build from source</h3>
              <p>Clone the repository and build with CMake and a C++20 compiler.</p>
            </div>
            <Link className="cardButton secondary" href="/docs/getting-started#build">
              Build guide
            </Link>
          </article>
        </div>
      </section>

      <section className="section">
        <div className="sectionHeader compact">
          <span className="eyebrow">use cases</span>
          <h2>Where ShakyLine fits</h2>
        </div>
        <div className="caseGrid">
          {useCases.map(([title, copy]) => (
            <article key={title}>
              <h3>{title}</h3>
              <p>{copy}</p>
            </article>
          ))}
        </div>
      </section>

      <section className="section split" id="usage">
        <div className="sectionHeader compact">
          <span className="eyebrow">workflow</span>
          <h2>A simple control loop</h2>
        </div>
        <div className="workflow">
          {workflow.map(([step, title, copy]) => (
            <article key={step}>
              <span>{step}</span>
              <h3>{title}</h3>
              <p>{copy}</p>
            </article>
          ))}
        </div>
      </section>

      <section className="section twoColumn">
        <div>
          <span className="eyebrow">documentation</span>
          <h2>Guides for real fault drills.</h2>
          <p>
            The docs include install paths, command recipes, API examples,
            profile fields, and practical test scenarios.
          </p>
          <div className="docCards">
            <Link href="/docs/getting-started">Getting started</Link>
            <Link href="/docs/use-cases">Use-case playbooks</Link>
            <Link href="/docs/api">Control API reference</Link>
            <Link href="/docs/faults">Fault profile guide</Link>
          </div>
        </div>
        <CodeBlock label="verify install">
          {`shakyline --version

shakyline --listen 0.0.0.0:8080 \\
  --upstream 127.0.0.1:9000 \\
  --control 9090`}
        </CodeBlock>
      </section>

      <Footer />
    </main>
  );
}
