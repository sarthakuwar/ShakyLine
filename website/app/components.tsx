import Link from "next/link";
import type { ReactNode } from "react";

export const links = {
  repo: "https://github.com/sarthakuwar/ShakyLine",
  latestRelease: "https://github.com/sarthakuwar/ShakyLine/releases/tag/v1.0.0",
  windowsInstaller:
    "https://github.com/sarthakuwar/ShakyLine/releases/download/v1.0.0/shakyline-1.0.0-windows-x64.exe",
  windowsZip:
    "https://github.com/sarthakuwar/ShakyLine/releases/download/v1.0.0/shakyline-1.0.0-windows-x64.zip",
  sourceZip: "https://github.com/sarthakuwar/ShakyLine/archive/refs/tags/v1.0.0.zip",
  sourceTar: "https://github.com/sarthakuwar/ShakyLine/archive/refs/tags/v1.0.0.tar.gz"
};

export function Header() {
  return (
    <header className="topbar">
      <Link className="brand" href="/" aria-label="ShakyLine home">
        <span className="brandMark">SL</span>
        ShakyLine
      </Link>
      <nav aria-label="Primary navigation">
        <Link href="/#download">Download</Link>
        <Link href="/docs/getting-started">Start</Link>
        <Link href="/docs/use-cases">Use Cases</Link>
        <Link href="/docs/api">API</Link>
        <Link href="/docs/faults">Faults</Link>
      </nav>
      <ExternalLink className="navButton" href={links.repo}>
        GitHub
      </ExternalLink>
    </header>
  );
}

export function ExternalLink({
  href,
  children,
  className = ""
}: {
  href: string;
  children: ReactNode;
  className?: string;
}) {
  return (
    <a className={className} href={href} rel="noreferrer" target="_blank">
      {children}
    </a>
  );
}

export function CodeBlock({ children, label }: { children: string; label: string }) {
  return (
    <div className="terminal">
      <div className="terminalChrome">
        <span />
        <span />
        <span />
        <strong>{label}</strong>
      </div>
      <pre>
        <code>{children}</code>
      </pre>
    </div>
  );
}

export function Footer() {
  return (
    <footer>
      <span>ShakyLine documentation portal</span>
      <div>
        <Link href="/docs">Docs</Link>
        <ExternalLink href={links.repo}>Repository</ExternalLink>
        <ExternalLink href={links.latestRelease}>Release v1.0.0</ExternalLink>
      </div>
    </footer>
  );
}

export function DocLayout({
  eyebrow,
  title,
  description,
  children
}: {
  eyebrow: string;
  title: string;
  description: string;
  children: ReactNode;
}) {
  return (
    <main>
      <Header />
      <section className="docHero">
        <span className="eyebrow">{eyebrow}</span>
        <h1>{title}</h1>
        <p>{description}</p>
      </section>
      <section className="docShell">
        <aside className="docNav">
          <Link href="/docs">Overview</Link>
          <Link href="/docs/getting-started">Getting Started</Link>
          <Link href="/docs/use-cases">Use Cases</Link>
          <Link href="/docs/api">Control API</Link>
          <Link href="/docs/faults">Fault Profiles</Link>
        </aside>
        <div className="docContent">{children}</div>
      </section>
      <Footer />
    </main>
  );
}
