import { CodeBlock, DocLayout, ExternalLink, links } from "../../components";

export default function GettingStarted() {
  return (
    <DocLayout
      eyebrow="guide"
      title="Getting started"
      description="Install ShakyLine, start a local proxy, and run a controlled network failure in a few commands."
    >
      <section className="docSection">
        <h2>Install</h2>
        <div className="downloadGrid three">
          <article className="downloadCard accent">
            <span className="platform">Windows</span>
            <h3>Installer</h3>
            <p>Best for normal users. Installs ShakyLine and adds it to PATH.</p>
            <ExternalLink className="cardButton" href={links.windowsInstaller}>
              Download installer
            </ExternalLink>
          </article>
          <article className="downloadCard">
            <span className="platform">Windows</span>
            <h3>Portable ZIP</h3>
            <p>Best for throwaway folders, CI images, or manual PATH setup.</p>
            <ExternalLink className="cardButton secondary" href={links.windowsZip}>
              Download ZIP
            </ExternalLink>
          </article>
          <article className="downloadCard">
            <span className="platform">Source</span>
            <h3>Linux / macOS</h3>
            <p>Build locally with CMake and a C++20-capable compiler.</p>
            <ExternalLink className="cardButton secondary" href={links.repo}>
              Open repository
            </ExternalLink>
          </article>
        </div>
      </section>

      <section className="docSection" id="build">
        <h2>Build from source</h2>
        <p>Use this path for Linux, macOS, or when you want to modify ShakyLine.</p>
        <CodeBlock label="build">
          {`git clone https://github.com/sarthakuwar/ShakyLine.git
cd ShakyLine
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release`}
        </CodeBlock>
        <div className="callout">
          On Windows with Visual Studio, the binary usually lands at
          <code> .\\build\\Release\\shakyline.exe</code>. On Linux and macOS,
          it usually lands at <code>./build/shakyline</code>.
        </div>
      </section>

      <section className="docSection">
        <h2>Run your first proxy</h2>
        <p>
          This listens on port 8080, forwards traffic to an upstream service,
          and exposes the control API on port 9090.
        </p>
        <CodeBlock label="start proxy">
          {`shakyline --listen 0.0.0.0:8080 \\
  --upstream api.example.com:443 \\
  --control 9090`}
        </CodeBlock>
      </section>

      <section className="docSection">
        <h2>Apply and remove a fault</h2>
        <CodeBlock label="fault drill">
          {`curl -X POST http://localhost:9090/profiles/default \\
  -H "Content-Type: application/json" \\
  -d '{"c2s_latency_ms":200,"c2s_drop_rate":0.05,"s2c_latency_ms":100}'

curl http://localhost:9090/sessions
curl http://localhost:9090/metrics

curl -X DELETE http://localhost:9090/profiles/default`}
        </CodeBlock>
      </section>
    </DocLayout>
  );
}
