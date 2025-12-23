#!/bin/env python3
import os
import sys
import argparse
import subprocess
from jinja2 import Environment, FileSystemLoader
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(description='Generate autoexec files from Jinja2 templates')
    parser.add_argument('-t', '--template', required=True, help='Template file name')
    parser.add_argument('-o', '--output', required=True, help='Output file path')
    parser.add_argument(
        '--scale',
        choices=['linear', 'logf'],
        default='linear',
        help='Scale type for the gauge (default: linear)'
    )

    args = parser.parse_args()

    # Setup Jinja2 environment
    template_dir = Path(__file__).parent.parent / 'firmware' / 'autoexec_templates'
    env = Environment(loader=FileSystemLoader(template_dir))

    # Template variables
    context = {}
    context['config'] = {}
    context['config']['scale'] = args.scale

    # Load and render template
    template = env.get_template(args.template)
    rendered = template.render(context)

    # Write output
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(rendered)

    print(f"Generated {args.output} from template {args.template}")

if __name__ == '__main__':
    main()
